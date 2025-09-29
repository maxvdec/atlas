//
// text.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Text rendering definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_TEXT_H
#define ATLAS_TEXT_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <map>
#include <string>
#include <vector>
struct Character {
    unsigned int textureID;
    Size2d size;
    Position2d bearing;
    unsigned int advance;
};

typedef std::map<char, Character> FontAtlas;

struct Font {
    std::string name;
    FontAtlas atlas;
    int size;
    Resource resource;

    static Font fromResource(const std::string &fontName, Resource resource,
                             int fontSize);
    static Font &getFont(const std::string &fontName);
    void changeSize(int newSize);

  private:
    static std::vector<Font> fonts;
};

class Text : public GameObject {
  public:
    std::string content;
    Font font;
    Position2d position;
    Color color = Color::white();
    Text() {};
    Text(const std::string &text, const Font &font,
         Position2d position = {0, 0}, const Color &color = Color::white())
        : content(text), font(font), position(position), color(color) {}

    void initialize() override;
    void render(float dt) override;

  private:
    Id VAO, VBO;
    glm::mat4 projection;
    ShaderProgram shader;
};

#endif // ATLAS_TEXT_H