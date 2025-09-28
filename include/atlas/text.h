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

#include "atlas/units.h"
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

#endif // ATLAS_TEXT_H