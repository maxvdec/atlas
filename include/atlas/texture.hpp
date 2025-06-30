/*
 texture.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture utilities and definitions for Atlas
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_TEXTURE_HPP
#define ATLAS_TEXTURE_HPP

#include "atlas/workspace.hpp"
#include <atlas/units.hpp>
#include <glad/glad.h>

enum class RepeatMode { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };
enum class FilteringMode { Nearest, Linear };
enum class MipmapFilteringMode {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureType {
    Color,
    Specular,
};

struct Texture {
    unsigned int ID;
    Size2d size;
    RepeatMode repeatMode = RepeatMode::Repeat;
    FilteringMode filteringMode = FilteringMode::Linear;
    MipmapFilteringMode mipmapFilteringMode =
        MipmapFilteringMode::LinearMipmapLinear;
    Color borderColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    Resource image = Resource();
    TextureType type = TextureType::Color;

    void setProperties();
    void fromImage(Resource resc, TextureType type);
};

#endif // ATLAS_TEXTURE_HPP
