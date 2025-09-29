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
#include "ft2build.h"
#include <iostream>
#include FT_FREETYPE_H
#include <glad/glad.h>

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

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Font font;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                     face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
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
    Window *window = Window::mainWindow;
    projection = glm::ortho(0.0f, static_cast<float>(window->width), 0.0f,
                            static_cast<float>(window->height));

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Text,
                                               AtlasFragmentShader::Text);
}

void Text::render(float dt) {
    glUseProgram(shader.programId);
    shader.setUniform3f("textColor", color.r, color.g, color.b);
    shader.setUniformMat4f("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    std::string::const_iterator c;
    for (c = content.begin(); c != content.end(); c++) {
        Character ch = font.atlas[*c];

        float xpos = position.x + ch.bearing.x * 1.0f;
        float ypos = position.y - (ch.size.height - ch.bearing.y) * 1.0f;

        float w = ch.size.width * 1.0f;
        float h = ch.size.height * 1.0f;

        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},    {xpos, ypos, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 1.0f},

            {xpos, ypos + h, 0.0f, 0.0f},    {xpos + w, ypos, 1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f}};

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        position.x += (ch.advance >> 6) * 1.0f;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}