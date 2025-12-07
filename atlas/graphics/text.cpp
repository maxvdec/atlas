//
// text.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Text rendering implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/text.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include "ft2build.h" // IWYU pragma: keep
#include <algorithm>
#include <iostream>
#include FT_FREETYPE_H
#include <vector>

Font Font::fromResource(const std::string &fontName, Resource resource,
                        int fontSize) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("Could not initialize FreeType Library");
    }

    FT_Face face;
    if (FT_New_Face(ft, resource.path.c_str(), 0, &face)) {
        throw std::runtime_error("Failed to load font: " +
                                 std::string(resource.path));
    }

    int dpi = 96;
    FT_Set_Char_Size(face, 0, fontSize * 64, dpi, dpi);

    Font font;

    const int atlasWidth = 1024;
    const int atlasHeight = 1024;
    std::vector<unsigned char> atlasData(atlasWidth * atlasHeight, 0);

    int currentX = 0;
    int currentY = 0;
    int rowHeight = 0;
    const int padding = 1;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        int width = face->glyph->bitmap.width;
        int height = face->glyph->bitmap.rows;

        if (currentX + width + padding > atlasWidth) {
            currentX = 0;
            currentY += rowHeight + padding;
            rowHeight = 0;
        }

        if (currentY + height + padding > atlasHeight) {
            std::cerr << "Font atlas full for font: " << fontName << std::endl;
            break;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                atlasData[(currentY + y) * atlasWidth + (currentX + x)] =
                    face->glyph->bitmap.buffer[y * width + x];
            }
        }

        Character character = {
            .size = Size2d(width, height),
            .bearing =
                Position2d(face->glyph->bitmap_left, face->glyph->bitmap_top),
            .advance = static_cast<unsigned int>(face->glyph->advance.x),
            // UV Min (Top-Left)
            .uvMin = Position2d(static_cast<float>(currentX) / atlasWidth,
                                static_cast<float>(currentY) / atlasHeight),
            // UV Max (Bottom-Right)
            .uvMax = Position2d(
                static_cast<float>(currentX + width) / atlasWidth,
                static_cast<float>(currentY + height) / atlasHeight)};

        font.atlas.insert(std::pair<char, Character>(c, character));

        currentX += width + padding;
        rowHeight = std::max(rowHeight, height);
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Red8, atlasWidth,
        atlasHeight, opal::TextureDataFormat::Red, atlasData.data(), 1);

    opalTexture->setParameters(
        opal::TextureWrapMode::ClampToEdge, opal::TextureWrapMode::ClampToEdge,
        opal::TextureFilterMode::Linear, opal::TextureFilterMode::Linear);

    font.texture = opalTexture;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    font.name = fontName;
    font.size = fontSize;
    font.resource = resource;

    Font::fonts.push_back(font);
    return font;
}

Font &Font::getFont(const std::string &fontName) {
    for (auto &font : fonts) {
        if (font.name == fontName) {
            return font;
        }
    }
    throw std::runtime_error("Font not found: " + fontName);
}

void Font::changeSize(int newSize) {
    if (newSize == size) {
        return;
    }
    Font newFont = Font::fromResource(name, resource, newSize);
    atlas = newFont.atlas;
    texture = newFont.texture;
    size = newSize;
}

std::vector<Font> Font::fonts = {};

void Text::initialize() {
    for (auto &component : components) {
        component->init();
    }
    Window *window = Window::mainWindow;
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(static_cast<GLFWwindow *>(window->windowRef),
                           &fbWidth, &fbHeight);

#ifdef VULKAN
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    vertexBufferCapacity = sizeof(float) * 6 * 4; // one glyph quad
    vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                        vertexBufferCapacity, nullptr,
                                        opal::MemoryUsageType::CPUToGPU);
    vao = opal::DrawingState::create(vertexBuffer);
    vao->setBuffers(vertexBuffer, nullptr);

    opal::VertexAttribute textAttribute{
        .name = "textVertex",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 4,
        .stride = static_cast<uint>(4 * sizeof(float)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    std::vector<opal::VertexAttributeBinding> bindings = {
        {textAttribute, vertexBuffer}};
    vao->configureAttributes(bindings);

    shader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Text,
                                               AtlasFragmentShader::Text);
}

void Text::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                  bool updatePipeline) {
    (void)updatePipeline;
    for (auto &component : components) {
        component->update(dt);
    }
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "Text::render requires a valid command buffer");
    }

    static std::shared_ptr<opal::Pipeline> textPipeline = nullptr;
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(
        static_cast<GLFWwindow *>(Window::mainWindow->windowRef), &fbWidth,
        &fbHeight);

    if (textPipeline == nullptr) {
        textPipeline = opal::Pipeline::create();

        opal::VertexAttribute textAttribute{
            .name = "vertex",
            .type = opal::VertexAttributeType::Float,
            .offset = 0,
            .location = 0,
            .normalized = false,
            .size = 4,
            .stride = static_cast<uint>(4 * sizeof(float)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};
        std::vector<opal::VertexAttribute> textAttributes = {textAttribute};
        opal::VertexBinding textBinding{
            .stride = static_cast<uint>(4 * sizeof(float)),
            .inputRate = opal::VertexBindingInputRate::Vertex};
        textPipeline->setVertexAttributes(textAttributes, textBinding);
        textPipeline->setShaderProgram(shader.shader);
#ifdef VULKAN
        textPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        textPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        textPipeline->setCullMode(opal::CullMode::None);
        textPipeline->enableDepthTest(false);
        textPipeline->enableDepthWrite(false);
        textPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        textPipeline->enableBlending(true);
        textPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                   opal::BlendFunc::OneMinusSrcAlpha);
        textPipeline->build();
    } else {
#ifdef VULKAN
        textPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        textPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        textPipeline->setShaderProgram(shader.shader);
    }

    textPipeline->enableBlending(true);
    textPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                               opal::BlendFunc::OneMinusSrcAlpha);
    textPipeline->enableDepthTest(false);

    commandBuffer->bindPipeline(textPipeline);

    textPipeline->setUniform3f("textColor", color.r, color.g, color.b);
    textPipeline->setUniformMat4f("projection", projection);

    if (font.texture) {
        textPipeline->bindTexture2D("text", font.texture->textureID, 0);
    }

    commandBuffer->bindDrawingState(vao);

    glfwGetFramebufferSize(
        static_cast<GLFWwindow *>(Window::mainWindow->windowRef), &fbWidth,
        &fbHeight);
#ifdef VULKAN
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    float scale = 2.0f;

    float maxBearingY = 0;
    for (const char &ch : content) {
        Character character = font.atlas[ch];
        maxBearingY = std::max(character.bearing.y, maxBearingY);
    }

    float x = position.x;
    float y = position.y + (maxBearingY * scale);

    std::string::const_iterator c;
    const size_t glyphCount = content.size();
    const size_t bytesPerGlyph = sizeof(float) * 6 * 4;
    const size_t requiredBytes = glyphCount * bytesPerGlyph;
    if (requiredBytes > vertexBufferCapacity) {
        vertexBufferCapacity = requiredBytes;
        vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                            vertexBufferCapacity, nullptr,
                                            opal::MemoryUsageType::CPUToGPU);
        vao->setBuffers(vertexBuffer, nullptr);
    }

    size_t glyphIndex = 0;
    for (c = content.begin(); c != content.end(); c++, ++glyphIndex) {
        Character ch = font.atlas[*c];

        float xpos = x + (ch.bearing.x * scale);
        float ypos = y - (ch.bearing.y * scale);

        float w = ch.size.width * scale;
        float h = ch.size.height * scale;

        float u0 = ch.uvMin.x;
        float v0 = ch.uvMin.y;
        float u1 = ch.uvMax.x;
        float v1 = ch.uvMax.y;

        float vertices[6][4] = {
            {xpos, ypos, u0, v0},         {xpos, ypos + h, u0, v1},
            {xpos + w, ypos + h, u1, v1},

            {xpos, ypos, u0, v0},         {xpos + w, ypos + h, u1, v1},
            {xpos + w, ypos, u1, v0}};

        vertexBuffer->bind();
        const size_t offset = glyphIndex * bytesPerGlyph;
        vertexBuffer->updateData(offset, sizeof(vertices), vertices);
        vertexBuffer->unbind();
        commandBuffer->draw(6, 1,
                            static_cast<uint>(offset / (4 * sizeof(float))), 0);

        x += (ch.advance >> 6) * scale;
    }

    commandBuffer->unbindDrawingState();
    textPipeline->enableBlending(false);
    textPipeline->enableDepthTest(true);
    textPipeline->bind();
}
