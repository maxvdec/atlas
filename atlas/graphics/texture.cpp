/*
 texture.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture importing functions and definitions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <algorithm>
#include <array>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Texture Texture::fromResourceName(const std::string &resourceName,
                                  TextureType type, TextureParameters params,
                                  Color borderColor) {
    Resource resource = Workspace::get().getResource(resourceName);
    return fromResource(resource, type, params, borderColor);
}

Texture Texture::fromResource(const Resource &resource, TextureType type,
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

    TextureCreationData creationData{};

    if (type == TextureType::HDR) {
        float *data = stbi_loadf(resource.path.string().c_str(), &width,
                                 &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load HDR image: " +
                                     resource.path.string());
        }

        creationData = TextureCreationData{width, height, channels};

        GLenum internalFormat = GL_RGB16F;
        GLenum dataFormat = GL_RGB;
        if (channels == 1) {
            internalFormat = GL_R16F;
            dataFormat = GL_RED;
        } else if (channels == 4) {
            internalFormat = GL_RGBA16F;
            dataFormat = GL_RGBA;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                     dataFormat, GL_FLOAT, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    } else {
        unsigned char *data = stbi_load(resource.path.string().c_str(), &width,
                                        &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load image: " +
                                     resource.path.string());
        }

        creationData = TextureCreationData{width, height, channels};

        glTexImage2D(GL_TEXTURE_2D, 0,
                     channels == 4 ? GL_SRGB8_ALPHA8
                                   : (channels == 3 ? GL_SRGB8 : GL_R8),
                     width, height, 0,
                     channels == 4 ? GL_RGBA
                                   : (channels == 3 ? GL_RGB : GL_RED),
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    Texture texture{resource, creationData, textureId, type, borderColor};
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

Cubemap Cubemap::fromResourceGroup(ResourceGroup &group) {
    if (group.resources.size() != 6) {
        throw std::runtime_error("Cubemap requires exactly 6 resources");
    }

    Id textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    int width = 0, height = 0, channels = 0;
    glm::dvec3 accumulatedColor(0.0);
    unsigned long long accumulatedPixels = 0;

    for (size_t i = 0; i < 6; i++) {
        Resource &resource = group.resources[i];
        if (resource.type != ResourceType::Image) {
            throw std::runtime_error("Resource is not an image: " +
                                     resource.name);
        }

        int w, h, c;
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data =
            stbi_load(resource.path.string().c_str(), &w, &h, &c, 0);

        if (!data) {
            throw std::runtime_error("Failed to load image: " +
                                     resource.path.string());
        }

        if (i == 0) {
            width = w;
            height = h;
            channels = c;
        } else {
            if (w != width || h != height || c != channels) {
                stbi_image_free(data);
                throw std::runtime_error(
                    "All cubemap images must have the same dimensions and "
                    "channels");
            }
        }

        unsigned long long facePixelCount = static_cast<unsigned long long>(w) *
                                            static_cast<unsigned long long>(h);
        if (facePixelCount > 0) {
            glm::dvec3 faceSum(0.0);
            if (c >= 3) {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    unsigned char *pixelPtr = data + pixel * c;
                    faceSum.x += pixelPtr[0];
                    faceSum.y += pixelPtr[1];
                    faceSum.z += pixelPtr[2];
                }
            } else if (c == 1) {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    unsigned char value = data[pixel];
                    faceSum += glm::dvec3(value, value, value);
                }
            }
            accumulatedColor += faceSum;
            accumulatedPixels += facePixelCount;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                     c == 4 ? GL_RGBA : (c == 3 ? GL_RGB : GL_RED), w, h, 0,
                     c == 4 ? GL_RGBA : (c == 3 ? GL_RGB : GL_RED),
                     GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    TextureCreationData creationData{width, height, channels};
    Cubemap cubemap;
    cubemap.creationData = creationData;
    cubemap.id = textureId;
    std::array<Resource, 6> resources;
    std::copy(group.resources.begin(), group.resources.end(),
              resources.begin());
    cubemap.resources = resources;
    if (accumulatedPixels > 0) {
        glm::dvec3 normalized =
            accumulatedColor / (static_cast<double>(accumulatedPixels) * 255.0);
        cubemap.averageColor = {normalized.x, normalized.y, normalized.z, 1.0};
        cubemap.hasAverageColor = true;
    }
    return cubemap;
}

void Skybox::display(Window &window) {
    CoreObject obj;

    std::vector<CoreVertex> vertices = {
        // Positions (x, y, z)
        CoreVertex({-1.0f, 1.0f, -1.0f}),  // 0
        CoreVertex({-1.0f, -1.0f, -1.0f}), // 1
        CoreVertex({1.0f, -1.0f, -1.0f}),  // 2
        CoreVertex({1.0f, 1.0f, -1.0f}),   // 3
        CoreVertex({-1.0f, 1.0f, 1.0f}),   // 4
        CoreVertex({-1.0f, -1.0f, 1.0f}),  // 5
        CoreVertex({1.0f, -1.0f, 1.0f}),   // 6
        CoreVertex({1.0f, 1.0f, 1.0f})     // 7
    };

    std::vector<Index> indices = {// Back face
                                  0, 1, 2, 2, 3, 0,
                                  // Front face
                                  4, 7, 6, 6, 5, 4,
                                  // Left face
                                  4, 5, 1, 1, 0, 4,
                                  // Right face
                                  3, 2, 6, 6, 7, 3,
                                  // Bottom face
                                  1, 5, 6, 6, 2, 1,
                                  // Top face
                                  4, 0, 3, 3, 7, 4};

    obj.attachVertices(vertices);
    obj.attachIndices(indices);

    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Skybox);
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Skybox);

    obj.createAndAttachProgram(vertexShader, fragmentShader);

    obj.initialize();

    this->object = std::make_shared<CoreObject>(obj);

    window.addPreludeObject(this);
}

void Skybox::hide() {
    if (object != nullptr) {
        this->object->hide();
    }
}

void Skybox::show() {
    if (object != nullptr) {
        this->object->show();
    }
}

void Skybox::setViewMatrix(const glm::mat4 &view) {
    this->view = glm::mat4(glm::mat3(view));
    if (object != nullptr) {
        object->setViewMatrix(this->view);
    }
}

void Skybox::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
    if (object != nullptr) {
        object->setProjectionMatrix(this->projection);
    }
}

void Skybox::render(float dt) {
    if (!object || !object->isVisible) {
        return;
    }

    CoreObject *obj = this->object.get();

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(obj->shaderProgram.programId);

    obj->shaderProgram.setUniformMat4f("view", view);

    obj->shaderProgram.setUniformMat4f("projection", projection);

    obj->shaderProgram.setUniform1i("skybox", 0);

    glBindVertexArray(obj->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(obj->indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // Set depth function back to default
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}
