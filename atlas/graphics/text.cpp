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

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        if (face->glyph->bitmap.width == 0 || face->glyph->bitmap.rows == 0) {
            Character character = {
                0, nullptr, Size2d(0, 0), Position2d(0, 0),
                static_cast<unsigned int>(face->glyph->advance.x)};
            font.atlas.insert(std::pair<char, Character>(c, character));
            continue;
        }

        auto opalTexture = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Red8,
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            opal::TextureDataFormat::Red, face->glyph->bitmap.buffer, 1);

        opalTexture->setParameters(opal::TextureWrapMode::ClampToEdge,
                                   opal::TextureWrapMode::ClampToEdge,
                                   opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);

        Character character = {
            opalTexture->textureID, opalTexture,
            Size2d(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            Position2d(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)};

        font.atlas.insert(std::pair<char, Character>(c, character));
    }

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

    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);

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

    static bool debugOnce = true;
    if (debugOnce) {
        std::cout << "[TEXT DEBUG] Text::render called, content: '" << content
                  << "'" << std::endl;
        std::cout << "[TEXT DEBUG] Position: (" << position.x << ", "
                  << position.y << ")" << std::endl;
        std::cout << "[TEXT DEBUG] Color: (" << color.r << ", " << color.g
                  << ", " << color.b << ")" << std::endl;
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
        // Keep viewport in sync with current framebuffer size.
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

    commandBuffer->bindDrawingState(vao);

    // Recompute projection to follow live framebuffer size (resizes).
    glfwGetFramebufferSize(
        static_cast<GLFWwindow *>(Window::mainWindow->windowRef), &fbWidth,
        &fbHeight);
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);

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

        // Flip V to keep glyphs upright (FreeType buffer is top-left).
        float vertices[6][4] = {
            {xpos, ypos, 0.0f, 1.0f},         {xpos, ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f},

            {xpos, ypos, 0.0f, 1.0f},         {xpos + w, ypos + h, 1.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 1.0f}};

        textPipeline->bindTexture2D("text", ch.textureID, 0);
        vertexBuffer->bind();
        const size_t offset = glyphIndex * bytesPerGlyph;
        vertexBuffer->updateData(offset, sizeof(vertices), vertices);
        vertexBuffer->unbind();
        // firstVertex is offset / stride (4 floats per vertex)
        commandBuffer->draw(6, 1,
                            static_cast<uint>(offset / (4 * sizeof(float))), 0);

        x += (ch.advance >> 6) * scale;
    }

    if (debugOnce) {
        std::cout << "[TEXT DEBUG] Text render complete" << std::endl;
        debugOnce = false;
    }

    commandBuffer->unbindDrawingState();
    textPipeline->enableBlending(false);
    textPipeline->enableDepthTest(true);
    textPipeline->bind();
}
