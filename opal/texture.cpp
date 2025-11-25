//
// texture.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Texture support for Opal
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include <memory>

namespace opal {

std::shared_ptr<Texture> Texture::create(TextureType type, TextureFormat format,
                                         int width, int height,
                                         TextureDataFormat dataFormat,
                                         const void *data, uint mipLevels) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = type;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    GLenum textureType = GL_TEXTURE_2D;
    switch (type) {
    case TextureType::Texture2D:
        textureType = GL_TEXTURE_2D;
        break;
    case TextureType::TextureCubeMap:
        textureType = GL_TEXTURE_CUBE_MAP;
        break;
    case TextureType::Texture3D:
        textureType = GL_TEXTURE_3D;
        break;
    case TextureType::Texture2DArray:
        textureType = GL_TEXTURE_2D_ARRAY;
        break;
    default:
        textureType = GL_TEXTURE_2D;
        break;
    }

    glGenTextures(1, &texture->textureID);
    glBindTexture(textureType, texture->textureID);
    GLenum glFormat = GL_RGBA;
    switch (format) {
    case TextureFormat::Rgba8:
        glFormat = GL_RGBA8;
        break;
    case TextureFormat::Rgba16F:
        glFormat = GL_RGBA16F;
        break;
    case TextureFormat::Rgb8:
        glFormat = GL_RGB8;
        break;
    case TextureFormat::Rgb16F:
        glFormat = GL_RGB16F;
        break;
    case TextureFormat::Depth24Stencil8:
        glFormat = GL_DEPTH24_STENCIL8;
        break;
    case TextureFormat::Depth32F:
        glFormat = GL_DEPTH_COMPONENT32F;
        break;
    case TextureFormat::Red8:
        glFormat = GL_R8;
        break;
    case TextureFormat::Red16F:
        glFormat = GL_R16F;
        break;
    case TextureFormat::sRgba8:
        glFormat = GL_SRGB8_ALPHA8;
        break;
    case TextureFormat::sRgb8:
        glFormat = GL_SRGB8;
        break;
    default:
        glFormat = GL_RGBA;
        break;
    }

    GLenum glDataFormat = GL_RGBA;
    switch (dataFormat) {
    case TextureDataFormat::Rgba:
        glDataFormat = GL_RGBA;
        break;
    case TextureDataFormat::Rgb:
        glDataFormat = GL_RGB;
        break;
    case TextureDataFormat::Red:
        glDataFormat = GL_RED;
        break;
    case TextureDataFormat::Bgr:
        glDataFormat = GL_BGR;
        break;
    case TextureDataFormat::Bgra:
        glDataFormat = GL_BGRA;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }

    texture->glType = textureType;
    texture->glFormat = glFormat;

    if (textureType == GL_TEXTURE_2D && data != nullptr) {
        glTexImage2D(textureType, 0, glFormat, width, height, 0, glDataFormat,
                     GL_UNSIGNED_BYTE, data);

        if (mipLevels > 1) {
            glGenerateMipmap(textureType);
        }
    }
    return texture;
#endif
}

void Texture::updateFace(int faceIndex, const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    if (glFormat == 0 || type != TextureType::TextureCubeMap) {
        throw std::runtime_error("Texture::updateFace called on non-cubemap "
                                 "texture or uninitialized texture.");
    }
    GLenum glDataFormat = GL_RGBA;
    switch (dataFormat) {
    case TextureDataFormat::Rgba:
        glDataFormat = GL_RGBA;
        break;
    case TextureDataFormat::Rgb:
        glDataFormat = GL_RGB;
        break;
    case TextureDataFormat::Red:
        glDataFormat = GL_RED;
        break;
    case TextureDataFormat::Bgr:
        glDataFormat = GL_BGR;
        break;
    case TextureDataFormat::Bgra:
        glDataFormat = GL_BGRA;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, glFormat, width,
                 height, 0, glDataFormat, GL_UNSIGNED_BYTE, data);
#endif
}

void Texture::updateData3D(const void *data, int width, int height, int depth,
                           TextureDataFormat dataFormat) {
#ifdef OPENGL
    if (glFormat == 0 || type != TextureType::Texture3D) {
        throw std::runtime_error("Texture::updateData3D called on non-3D "
                                 "texture or uninitialized texture.");
    }

    GLenum glDataFormat = GL_RGBA;
    switch (dataFormat) {
    case TextureDataFormat::Rgba:
        glDataFormat = GL_RGBA;
        break;
    case TextureDataFormat::Rgb:
        glDataFormat = GL_RGB;
        break;
    case TextureDataFormat::Red:
        glDataFormat = GL_RED;
        break;
    case TextureDataFormat::Bgr:
        glDataFormat = GL_BGR;
        break;
    case TextureDataFormat::Bgra:
        glDataFormat = GL_BGRA;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }

    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0,
                 glDataFormat, GL_UNSIGNED_BYTE, data);
#endif
}

void Texture::updateData(const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    if (glFormat == 0 || type != TextureType::Texture2D) {
        throw std::runtime_error("Texture::updateData called on non-2D "
                                 "texture or uninitialized texture.");
    }
    GLenum glDataFormat = GL_RGBA;
    switch (dataFormat) {
    case TextureDataFormat::Rgba:
        glDataFormat = GL_RGBA;
        break;
    case TextureDataFormat::Rgb:
        glDataFormat = GL_RGB;
        break;
    case TextureDataFormat::Red:
        glDataFormat = GL_RED;
        break;
    case TextureDataFormat::Bgr:
        glDataFormat = GL_BGR;
        break;
    case TextureDataFormat::Bgra:
        glDataFormat = GL_BGRA;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glDataFormat,
                 GL_UNSIGNED_BYTE, data);
#endif
}

void Texture::changeFormat(TextureFormat newFormat) {
#ifdef OPENGL
    if (glFormat == 0) {
        throw std::runtime_error(
            "Texture::changeFormat called on uninitialized texture.");
    }
    GLenum glNewFormat = GL_RGBA;
    switch (newFormat) {
    case TextureFormat::Rgba8:
        glNewFormat = GL_RGBA8;
        break;
    case TextureFormat::Rgba16F:
        glNewFormat = GL_RGBA16F;
        break;
    case TextureFormat::Rgb8:
        glNewFormat = GL_RGB8;
        break;
    case TextureFormat::Rgb16F:
        glNewFormat = GL_RGB16F;
        break;
    case TextureFormat::Depth24Stencil8:
        glNewFormat = GL_DEPTH24_STENCIL8;
        break;
    case TextureFormat::Depth32F:
        glNewFormat = GL_DEPTH_COMPONENT32F;
        break;
    case TextureFormat::Red8:
        glNewFormat = GL_R8;
        break;
    case TextureFormat::Red16F:
        glNewFormat = GL_R16F;
        break;
    case TextureFormat::sRgba8:
        glNewFormat = GL_SRGB8_ALPHA8;
        break;
    case TextureFormat::sRgb8:
        glNewFormat = GL_SRGB8;
        break;
    default:
        glNewFormat = GL_RGBA;
        break;
    }
    this->format = newFormat;
    this->glFormat = glNewFormat;
#endif
}

void Texture::generateMipmaps([[maybe_unused]] uint levels) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#endif
}

void Texture::automaticallyGenerateMipmaps() {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#endif
}

void Texture::setWrapMode(TextureAxis axis, TextureWrapMode mode) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLenum glMode = GL_REPEAT;
    switch (mode) {
    case TextureWrapMode::Repeat:
        glMode = GL_REPEAT;
        break;
    case TextureWrapMode::MirroredRepeat:
        glMode = GL_MIRRORED_REPEAT;
        break;
    case TextureWrapMode::ClampToEdge:
        glMode = GL_CLAMP_TO_EDGE;
        break;
    case TextureWrapMode::ClampToBorder:
        glMode = GL_CLAMP_TO_BORDER;
        break;
    default:
        glMode = GL_REPEAT;
        break;
    }
    if (axis == TextureAxis::S) {
        glTexParameteri(this->glType, GL_TEXTURE_WRAP_S, glMode);
    } else if (axis == TextureAxis::T) {
        glTexParameteri(this->glType, GL_TEXTURE_WRAP_T, glMode);
    } else if (axis == TextureAxis::R) {
        if (this->type != TextureType::Texture3D &&
            this->type != TextureType::TextureCubeMap) {
            throw std::runtime_error(
                "Texture::setWrapMode called with R axis on non-3D/cubemap "
                "texture.");
        }
        glTexParameteri(this->glType, GL_TEXTURE_WRAP_R, glMode);
    }
#endif
}

void Texture::changeBorderColor(const glm::vec4 &borderColor) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLfloat color[4] = {borderColor.r, borderColor.g, borderColor.b,
                        borderColor.a};
    glTexParameterfv(this->glType, GL_TEXTURE_BORDER_COLOR, color);
#endif
}

void Texture::setFilterMode(TextureFilterMode minFilter,
                            TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLenum glMinFilter = GL_LINEAR;
    GLenum glMagFilter = GL_LINEAR;
    switch (minFilter) {
    case TextureFilterMode::Nearest:
        glMinFilter = GL_NEAREST;
        break;
    case TextureFilterMode::Linear:
        glMinFilter = GL_LINEAR;
        break;
    case TextureFilterMode::NearestMipmapNearest:
        glMinFilter = GL_NEAREST_MIPMAP_NEAREST;
        break;
    case TextureFilterMode::LinearMipmapLinear:
        glMinFilter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    default:
        glMinFilter = GL_LINEAR;
        break;
    }
    switch (magFilter) {
    case TextureFilterMode::Nearest:
        glMagFilter = GL_NEAREST;
        break;
    case TextureFilterMode::Linear:
        glMagFilter = GL_LINEAR;
        break;
    default:
        glMagFilter = GL_LINEAR;
        break;
    }
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, glMinFilter);
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, glMagFilter);
#endif
}

void Pipeline::bindTexture(const std::string &name,
                           std::shared_ptr<Texture> texture, int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(texture->glType, texture->textureID);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#endif
}

} // namespace opal
