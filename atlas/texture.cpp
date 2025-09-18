/*
 texture.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture importing functions and definitions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.h"
#include "atlas/workspace.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <glad/glad.h>

Texture Texture::fromResourceName(const std::string &resourceName,
                                  TextureParameters params, Color borderColor) {
    Resource resource = Workspace::get().getResource(resourceName);
    return fromResource(resource, params, borderColor);
}

Texture Texture::fromResource(const Resource &resource,
                              TextureParameters params, Color borderColor) {
    if (resource.type != ResourceType::Image) {
        throw std::runtime_error("Resource is not an image");
    }

    Id textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    Texture::applyWrappingMode(params.wrappingModeS, GL_TEXTURE_WRAP_S);
    Texture::applyWrappingMode(params.wrappingModeT, GL_TEXTURE_WRAP_T);
    Texture::applyFilteringMode(params.minifyingFilter, true);
    Texture::applyFilteringMode(params.magnifyingFilter, false);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            throw std::runtime_error(
                "Border color values must be between 0 and 1");
        }
        float borderColorArr[4] = {static_cast<float>(borderColor.r),
                                   static_cast<float>(borderColor.g),
                                   static_cast<float>(borderColor.b),
                                   static_cast<float>(borderColor.a)};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                         borderColorArr);
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(resource.path.string().c_str(), &width,
                                    &height, &channels, 0);
    TextureCreationData creationData{width, height, channels};

    if (!data) {
        throw std::runtime_error("Failed to load image: " +
                                 resource.path.string());
    }

    glTexImage2D(GL_TEXTURE_2D, 0,
                 channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : GL_RED),
                 width, height, 0,
                 channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : GL_RED),
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    Texture texture{resource, creationData, textureId, borderColor};
    return texture;
}

void Texture::applyWrappingMode(TextureWrappingMode mode, Id glAxis) {
    switch (mode) {
    case TextureWrappingMode::Repeat:
        glTexParameteri(GL_TEXTURE_2D, glAxis, GL_REPEAT);
        break;
    case TextureWrappingMode::MirroredRepeat:
        glTexParameteri(GL_TEXTURE_2D, glAxis, GL_MIRRORED_REPEAT);
        break;
    case TextureWrappingMode::ClampToEdge:
        glTexParameteri(GL_TEXTURE_2D, glAxis, GL_CLAMP_TO_EDGE);
        break;
    case TextureWrappingMode::ClampToBorder:
        glTexParameteri(GL_TEXTURE_2D, glAxis, GL_CLAMP_TO_BORDER);
        break;
    default:
        throw std::runtime_error("Unknown wrapping mode");
    }
}

void Texture::applyFilteringMode(TextureFilteringMode mode, bool isMinifying) {
    if (isMinifying) {
        switch (mode) {
        case TextureFilteringMode::Nearest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST_MIPMAP_NEAREST);
            break;
        case TextureFilteringMode::Linear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            break;
        default:
            throw std::runtime_error("Unknown filtering mode");
        }
    } else {
        switch (mode) {
        case TextureFilteringMode::Nearest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case TextureFilteringMode::Linear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        default:
            throw std::runtime_error("Unknown filtering mode");
        }
    }
}
