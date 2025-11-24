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
#include "ft2build.h" // IWYU pragma: keep
#include <iostream>
#include FT_FREETYPE_H
#include <glad/glad.h>
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

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Font font;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        if (face->glyph->bitmap.width == 0 || face->glyph->bitmap.rows == 0) {
            Character character = {
                0, Size2d(0, 0), Position2d(0, 0),
                static_cast<unsigned int>(face->glyph->advance.x)};
            font.atlas.insert(std::pair<char, Character>(c, character));
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, face->glyph->bitmap.width,
                     face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        Character character = {
            texture,
            Size2d(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            Position2d(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)};

        font.atlas.insert(std::pair<char, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);

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

    vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                        sizeof(float) * 6 * 4, nullptr,
                                        opal::MemoryUsageType::CPUToGPU);
    vao = opal::DrawingState::create(vertexBuffer);
    vao->setBuffers(vertexBuffer, nullptr);

    opal::VertexAttribute textAttribute{"textVertex",
                                        opal::VertexAttributeType::Float,
                                        0,
                                        0,
                                        false,
                                        4,
                                        static_cast<uint>(4 * sizeof(float)),
                                        opal::VertexBindingInputRate::Vertex,
                                        0};
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(shader.programId);
    shader.setUniform3f("textColor", color.r, color.g, color.b);
    shader.setUniformMat4f("projection", projection);
    shader.setUniform1i("text", 0);

    glActiveTexture(GL_TEXTURE0);
    commandBuffer->bindDrawingState(vao);

    float scale = 2.0f;

    float maxBearingY = 0;
    for (const char &ch : content) {
        Character character = font.atlas[ch];
        if (character.bearing.y > maxBearingY) {
            maxBearingY = character.bearing.y;
        }
    }

    float x = position.x;
    float y = position.y + maxBearingY * scale;

    std::string::const_iterator c;
    for (c = content.begin(); c != content.end(); c++) {
        Character ch = font.atlas[*c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - ch.bearing.y * scale;

        float w = ch.size.width * scale;
        float h = ch.size.height * scale;

        float vertices[6][4] = {
            {xpos, ypos, 0.0f, 0.0f},         {xpos, ypos + h, 0.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 1.0f},

            {xpos, ypos, 0.0f, 0.0f},         {xpos + w, ypos + h, 1.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 0.0f}};

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        vertexBuffer->bind();
        vertexBuffer->updateData(0, sizeof(vertices), vertices);
        vertexBuffer->unbind();
        commandBuffer->draw(6, 1, 0, 0);

        x += (ch.advance >> 6) * scale;
    }

    commandBuffer->unbindDrawingState();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
