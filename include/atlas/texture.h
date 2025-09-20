/*
 texture.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture definition and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include "atlas/units.h"
#include "atlas/workspace.h"
#include <string>

struct TextureCreationData {
    int width = 0;
    int height = 0;
    int channels = 0;
};

enum class TextureWrappingMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilteringMode { Nearest, Linear };

struct TextureParameters {
    TextureWrappingMode wrappingModeS = TextureWrappingMode::Repeat;
    TextureWrappingMode wrappingModeT = TextureWrappingMode::Repeat;
    TextureFilteringMode minifyingFilter = TextureFilteringMode::Linear;
    TextureFilteringMode magnifyingFilter = TextureFilteringMode::Linear;
};

enum class TextureType : int { Color = 0, Specular = 1 };

struct Texture {
    Resource resource;
    TextureCreationData creationData;
    Id id;
    TextureType type;
    Color borderColor = {0, 0, 0, 0};

    static Texture fromResource(const Resource &resource,
                                TextureType type = TextureType::Color,
                                TextureParameters params = {},
                                Color borderColor = {0, 0, 0, 0});
    static Texture fromResourceName(const std::string &resourceName,
                                    TextureType type = TextureType::Color,
                                    TextureParameters params = {},
                                    Color borderColor = {0, 0, 0, 0});

  private:
    static void applyWrappingMode(TextureWrappingMode mode, Id glAxis);
    static void applyFilteringMode(TextureFilteringMode mode, bool isMinifying);
};

#endif // TEXTURE_H
