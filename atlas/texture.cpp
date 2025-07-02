/*
 texture.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture definitions and tools
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/window.hpp"
#include "atlas/workspace.hpp"
#include <iostream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "atlas/texture.hpp"
#include <glad/glad.h>

void Texture::setProperties() {
    switch (this->repeatMode) {
    case RepeatMode::Repeat:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    case RepeatMode::MirroredRepeat:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        break;
    case RepeatMode::ClampToEdge:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case RepeatMode::ClampToBorder:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                         &this->borderColor.r);
        break;
    }

    switch (this->filteringMode) {
    case FilteringMode::Nearest:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case FilteringMode::Linear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    switch (this->mipmapFilteringMode) {
    case MipmapFilteringMode::Nearest:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case MipmapFilteringMode::Linear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case MipmapFilteringMode::NearestMipmapNearest:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST_MIPMAP_NEAREST);
        break;
    case MipmapFilteringMode::LinearMipmapNearest:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_NEAREST);
        break;
    case MipmapFilteringMode::NearestMipmapLinear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST_MIPMAP_LINEAR);
        break;
    case MipmapFilteringMode::LinearMipmapLinear:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
}

void Texture::fromImage(Resource resc, TextureType type) {
    this->type = type;
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data =
        stbi_load(resc.path.c_str(), &width, &height, &nrChannels, 0);

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    this->setProperties();

    if (resc.getExtension() == ".png") {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    this->ID = textureID;
    this->image = resc;
}

void Cubemap::fromImages(CubemapPacket packet, TextureType type) {
    this->texture = Texture();
    this->texture.type = type;
    glGenTextures(1, &this->texture.ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture.ID);

    std::vector<Resource> resources = {packet.right, packet.left,
                                       packet.top,   packet.bottom,
                                       packet.front, packet.back};

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < resources.size(); i++) {
        int width, height, nrChannels;
        unsigned char *data = stbi_load(resources[i].path.c_str(), &width,
                                        &height, &nrChannels, 0);
        if (data) {
            if (resources[i].getExtension() == ".png") {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                             width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            } else {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                             width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            }
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap texture failed to load at path: "
                      << resources[i].path << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Texture::renderToScreen() {
    this->fullScreenObject = new CoreObject(presentFullScreenTexture(*this));
    if (this->type == TextureType::Depth) {
        this->fullScreenObject->fragmentShader =
            CoreShader(VISUALIZE_DEPTH_FRAG, CoreShaderType::Fragment);
    }
    this->fullScreenObject->initCore();
    this->dispatcher = [](CoreObject *object) {
        if (!object->program.has_value()) {
            std::cerr << "Error: Shader program not initialized for texture "
                         "rendering."
                      << std::endl;
            return;
        }

        glUseProgram(object->program.value().ID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, object->textures[0].ID);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "Error binding texture to shader program."
                      << std::endl;
            return;
        }

        object->program.value().setInt("uTexture1", 0);
        glBindVertexArray(object->attributes.VAO);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(object->vertices.size()));
    };
    Window::current_window->fullScreenTexture = *this;
}
