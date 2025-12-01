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
                                         GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY,
                                         GL_TEXTURE_2D_MULTISAMPLE};

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

    // For single-channel textures, save and set unpack alignment to 1
    GLint previousAlignment = 4;
    bool needsAlignmentFix = (dataFormat == TextureDataFormat::Red);
    if (needsAlignmentFix) {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

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

    // Restore previous alignment
    if (needsAlignmentFix) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
    }

    return texture;
#elif defined(VULKAN)
    return Texture::createVulkan(type, format, width, height, dataFormat, data,
                                 mipLevels);
#else
    return nullptr;
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
#elif defined(VULKAN)
    // Vulkan cubemap face update would require staging buffer and copy
    (void)faceIndex;
    (void)data;
    (void)width;
    (void)height;
    (void)dataFormat;
#endif
}

void Texture::updateData3D(const void *data, int width, int height, int depth,
                           TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0, glDataFmt,
                 GL_UNSIGNED_BYTE, data);
#elif defined(VULKAN)
    // Vulkan 3D texture update would require staging buffer and copy
    (void)data;
    (void)width;
    (void)height;
    (void)depth;
    (void)dataFormat;
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
#elif defined(VULKAN)
    // Vulkan texture update would require staging buffer and copy
    (void)data;
    (void)width;
    (void)height;
    (void)dataFormat;
#endif
}

void Texture::changeFormat(TextureFormat newFormat) {
#ifdef OPENGL
    this->format = newFormat;
    this->glFormat = getGLInternalFormat(newFormat);
#elif defined(VULKAN)
    this->format = newFormat;
    // Note: In Vulkan, changing format may require recreating the image
#endif
}

void Texture::readData(void *buffer, TextureDataFormat dataFormat) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLenum glDataFormat;
    GLenum glDataType = GL_UNSIGNED_BYTE;
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
    case TextureDataFormat::DepthComponent:
        glDataFormat = GL_DEPTH_COMPONENT;
        glDataType = GL_FLOAT;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }
    glGetTexImage(this->glType, 0, glDataFormat, glDataType, buffer);
#elif defined(VULKAN)
    // Vulkan readback requires transitioning to transfer src layout
    // and using a staging buffer
    (void)buffer;
    (void)dataFormat;
#endif
}

void Texture::generateMipmaps([[maybe_unused]] uint levels) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#elif defined(VULKAN)
    // Vulkan mipmap generation requires vkCmdBlitImage
#endif
}

void Texture::automaticallyGenerateMipmaps() {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#elif defined(VULKAN)
    // Vulkan mipmap generation requires vkCmdBlitImage
#endif
}

void Texture::setWrapMode(TextureAxis axis, TextureWrapMode mode) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    const GLenum glMode = getGLWrapMode(mode);
    static constexpr GLenum axisTable[] = {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
                                           GL_TEXTURE_WRAP_R};
    glTexParameteri(this->glType, axisTable[static_cast<int>(axis)], glMode);
#elif defined(VULKAN)
    // In Vulkan, sampler parameters are set at sampler creation time
    (void)axis;
    (void)mode;
#endif
}

void Texture::changeBorderColor(const glm::vec4 &borderColor) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLfloat color[4] = {borderColor.r, borderColor.g, borderColor.b,
                        borderColor.a};
    glTexParameterfv(this->glType, GL_TEXTURE_BORDER_COLOR, color);
#elif defined(VULKAN)
    // In Vulkan, border color is set at sampler creation time
    (void)borderColor;
#endif
}

void Texture::setFilterMode(TextureFilterMode minFilter,
                            TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#elif defined(VULKAN)
    // In Vulkan, filter modes are set at sampler creation time
    (void)minFilter;
    (void)magFilter;
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
#elif defined(VULKAN)
    // In Vulkan, these parameters are set at sampler creation time
    (void)wrapS;
    (void)wrapT;
    (void)minFilter;
    (void)magFilter;
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
#elif defined(VULKAN)
    // In Vulkan, these parameters are set at sampler creation time
    (void)wrapS;
    (void)wrapT;
    (void)wrapR;
    (void)minFilter;
    (void)magFilter;
#endif
}

void Pipeline::bindTexture(const std::string &name,
                           std::shared_ptr<Texture> texture, int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(texture->glType, texture->textureID);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#elif defined(VULKAN)
    // Texture binding for Vulkan requires descriptor set updates
    // This is handled through the descriptor set system
    // For now, silently ignore if uniform not found (matches OpenGL behavior)
    (void)name;
    (void)texture;
    (void)unit;
#endif
}

void Pipeline::bindTexture2D(const std::string &name, uint textureId,
                             int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#elif defined(VULKAN)
    // Vulkan texture binding requires descriptor sets - not yet implemented
    // Silently ignore if uniform not found (matches OpenGL behavior)
    (void)name;
    (void)textureId;
    (void)unit;
#endif
}

void Pipeline::bindTexture3D(const std::string &name, uint textureId,
                             int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#elif defined(VULKAN)
    // Vulkan texture binding requires descriptor sets - not yet implemented
    // Silently ignore if uniform not found (matches OpenGL behavior)
    (void)name;
    (void)textureId;
    (void)unit;
#endif
}

void Pipeline::bindTextureCubemap(const std::string &name, uint textureId,
                                  int unit) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#elif defined(VULKAN)
    // Vulkan texture binding requires descriptor sets - not yet implemented
    // Silently ignore if uniform not found (matches OpenGL behavior)
    (void)name;
    (void)textureId;
    (void)unit;
#endif
}

std::shared_ptr<Texture> Texture::createMultisampled(TextureFormat format,
                                                     int width, int height,
                                                     int samples) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture2DMultisample;
    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->samples = samples;

    const GLenum glFormat = getGLInternalFormat(format);
    texture->glType = GL_TEXTURE_2D_MULTISAMPLE;
    texture->glFormat = glFormat;

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture->textureID);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, glFormat, width,
                            height, GL_TRUE);

    return texture;
#elif defined(VULKAN)
    return Texture::createMultisampledVulkan(format, width, height, samples);
#else
    return nullptr;
#endif
}

std::shared_ptr<Texture> Texture::createDepthCubemap(TextureFormat format,
                                                     int resolution) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::TextureCubeMap;
    texture->format = format;
    texture->width = resolution;
    texture->height = resolution;

    const GLenum glFormat = getGLInternalFormat(format);
    texture->glType = GL_TEXTURE_CUBE_MAP;
    texture->glFormat = glFormat;

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureID);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glFormat,
                     resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                     nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texture;
#elif defined(VULKAN)
    return Texture::createDepthCubemapVulkan(format, resolution);
#else
    return nullptr;
#endif
}

std::shared_ptr<Texture> Texture::create3D(TextureFormat format, int width,
                                           int height, int depth,
                                           TextureDataFormat dataFormat,
                                           const void *data) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture3D;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    const GLenum glFormat = getGLInternalFormat(format);
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    texture->glType = GL_TEXTURE_3D;
    texture->glFormat = glFormat;

    // Determine data type based on format
    GLenum dataType = GL_UNSIGNED_BYTE;
    if (format == TextureFormat::Rgba16F || format == TextureFormat::Rgb16F ||
        format == TextureFormat::Red16F) {
        dataType = GL_FLOAT;
    }

    // Save and restore previous state
    GLint previousAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &previousTexture);

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_3D, texture->textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0, glDataFmt,
                 dataType, data);

    // Restore previous state
    glBindTexture(GL_TEXTURE_3D, previousTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    return texture;
#elif defined(VULKAN)
    return Texture::create3DVulkan(format, width, height, depth, dataFormat,
                                   data);
#else
    return nullptr;
#endif
}

} // namespace opal
