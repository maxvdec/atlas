/*
 skybox.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Skybox utilties and functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/scene.hpp"
#include "atlas/texture.hpp"
#include "atlas/window.hpp"
#include <iostream>
#include <vector>

void Skybox::addCubemap(Cubemap cubemap) {
    float skyboxVertices[108] = {
        // positions
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    CoreObject skyboxObject;
    this->cubemap = cubemap;
    this->object = skyboxObject;

    for (int i = 0; i < sizeof(skyboxVertices) / sizeof(float); i += 3) {
        CoreVertex vertex;
        vertex.x = skyboxVertices[i];
        vertex.y = skyboxVertices[i + 1];
        vertex.z = skyboxVertices[i + 2];
        vertex.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        this->object.value().vertices.push_back(vertex);
    }

    this->object.value().addTexture(this->cubemap->texture);
    object->fragmentShader = CoreShader(SKYBOX_FRAG, CoreShaderType::Fragment);
    object->vertexShader = CoreShader(SKYBOX_VERT, CoreShaderType::Vertex);

    object->initCore();
    auto dispatcher = [](CoreObject *obj) {
        GLint currentDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &currentDepthFunc);

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);

        obj->program->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, obj->textures[0].ID);
        glBindVertexArray(obj->attributes.VAO);
        obj->program->setMatrix4("uProjection", obj->projectionMatrix);
        obj->program->setMatrix4("uView",
                                 glm::mat4(glm::mat3(obj->viewMatrix)));
        obj->program->setInt("uSkyboxTexture", 0);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(obj->vertices.size()));

        glDepthMask(GL_TRUE);
        glDepthFunc(currentDepthFunc);
    };
    this->dispatcher = dispatcher;
}

void Skybox::useSkybox() {
    Window::current_window->currentScene->skybox = this;
}
