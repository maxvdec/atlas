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

namespace {

constexpr GLenum glInternalFormatTable[] = {GL_RGBA8,
                                            GL_SRGB8_ALPHA8,
                                            GL_RGB8,
                                            GL_SRGB8,
                                            GL_RGBA16F,
                                            GL_RGB16F,
                                            GL_DEPTH24_STENCIL8,
                                            GL_DEPTH_COMPONENT24,
                                            GL_DEPTH_COMPONENT32F,
                                            GL_R8,
                                            GL_R16F};

constexpr GLenum glDataFormatTable[] = {
    GL_RGBA, GL_RGB, GL_RED, GL_BGR, GL_BGRA, GL_DEPTH_COMPONENT,
};

constexpr GLenum glTextureTypeTable[] = {GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP,
                                         GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY};

constexpr GLenum glWrapModeTable[] = {GL_REPEAT, GL_MIRRORED_REPEAT,
                                      GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER};

constexpr GLenum glFilterModeTable[] = {
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR};

inline GLenum getGLInternalFormat(TextureFormat format) {
    return glInternalFormatTable[static_cast<int>(format)];
}

inline GLenum getGLDataFormat(TextureDataFormat format) {
    return glDataFormatTable[static_cast<int>(format)];
}

inline GLenum getGLTextureType(TextureType type) {
    return glTextureTypeTable[static_cast<int>(type)];
}

inline GLenum getGLWrapMode(TextureWrapMode mode) {
    return glWrapModeTable[static_cast<int>(mode)];
}

inline GLenum getGLFilterMode(TextureFilterMode mode) {
    return glFilterModeTable[static_cast<int>(mode)];
}

} // namespace

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

    const GLenum textureType = getGLTextureType(type);
    const GLenum glFormat = getGLInternalFormat(format);
    const GLenum glDataFmt = getGLDataFormat(dataFormat);

    texture->glType = textureType;
    texture->glFormat = glFormat;

    glGenTextures(1, &texture->textureID);
    glBindTexture(textureType, texture->textureID);

    if (textureType == GL_TEXTURE_2D && width > 0 && height > 0) {
        GLenum dataType = GL_UNSIGNED_BYTE;
        if (format == TextureFormat::Rgba16F ||
            format == TextureFormat::Rgb16F ||
            format == TextureFormat::Red16F) {
            dataType = GL_FLOAT;
        }
        glTexImage2D(textureType, 0, glFormat, width, height, 0, glDataFmt,
                     dataType, data);
        if (mipLevels > 1 && data != nullptr) {
            glGenerateMipmap(textureType);
        }
    } else if (textureType == GL_TEXTURE_CUBE_MAP && width > 0 && height > 0) {
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glFormat, width,
                         height, 0, glDataFmt, GL_UNSIGNED_BYTE, nullptr);
        }
    }
    return texture;
#endif
}

void Texture::updateFace(int faceIndex, const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    if (this->width == width && this->height == height) {
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, 0, 0,
                        width, height, glDataFmt, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, glFormat,
                     width, height, 0, glDataFmt, GL_UNSIGNED_BYTE, data);
    }
#endif
}

void Texture::updateData3D(const void *data, int width, int height, int depth,
                           TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0, glDataFmt,
                 GL_UNSIGNED_BYTE, data);
#endif
}

void Texture::updateData(const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    GLenum dataType = GL_UNSIGNED_BYTE;
    if (format == TextureFormat::Rgba16F || format == TextureFormat::Rgb16F ||
        format == TextureFormat::Red16F) {
        dataType = GL_FLOAT;
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (this->width == width && this->height == height) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, glDataFmt,
                        dataType, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glDataFmt,
                     dataType, data);
        this->width = width;
        this->height = height;
    }
#endif
}

void Texture::changeFormat(TextureFormat newFormat) {
#ifdef OPENGL
    this->format = newFormat;
    this->glFormat = getGLInternalFormat(newFormat);
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
    const GLenum glMode = getGLWrapMode(mode);
    static constexpr GLenum axisTable[] = {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
                                           GL_TEXTURE_WRAP_R};
    glTexParameteri(this->glType, axisTable[static_cast<int>(axis)], glMode);
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
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#endif
}

void Texture::setParameters(TextureWrapMode wrapS, TextureWrapMode wrapT,
                            TextureFilterMode minFilter,
                            TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_WRAP_S, getGLWrapMode(wrapS));
    glTexParameteri(glType, GL_TEXTURE_WRAP_T, getGLWrapMode(wrapT));
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#endif
}

void Texture::setParameters3D(TextureWrapMode wrapS, TextureWrapMode wrapT,
                              TextureWrapMode wrapR,
                              TextureFilterMode minFilter,
                              TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_WRAP_S, getGLWrapMode(wrapS));
    glTexParameteri(glType, GL_TEXTURE_WRAP_T, getGLWrapMode(wrapT));
    glTexParameteri(glType, GL_TEXTURE_WRAP_R, getGLWrapMode(wrapR));
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
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

void Pipeline::bindTexture2D(const std::string &name, uint textureId,
                             int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#endif
}

void Pipeline::bindTextureCubemap(const std::string &name, uint textureId,
                                  int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#endif
}

} // namespace opal
