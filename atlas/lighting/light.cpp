/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting functions and solutions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.hpp"
#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/texture.hpp"
#include "atlas/units.hpp"
#include "atlas/window.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>

Light::Light(Position3d position, Color color, LightType type, Scene *scene,
             float intensity)
    : position(position), color(color) {

    this->debugObject = generateCubeObject(position, Size3d(0.1f, 0.1f, 0.1f));
    this->intensity = intensity;
    for (int i = 0; i < debugObject.vertices.size(); ++i) {
        debugObject.setVertexColor(i, color);
    }

    debugObject.fragmentShader =
        CoreShader(NORMAL_FRAG, CoreShaderType::Fragment);

    debugObject.hide();
    debugObject.initialize();
    this->type = type;
    if (Window::current_window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    if (Window::current_window->currentScene == nullptr) {
        return;
    }
    Window::current_window->currentScene->useLight(this);
}

void Light::debugLight() { this->debugObject.show(); }

const std::vector<std::tuple<float, float, float, float>> attenuationTable = {
    {7, 1.0f, 0.7f, 1.8f},        {13, 1.0f, 0.35f, 0.44f},
    {20, 1.0f, 0.22f, 0.20f},     {32, 1.0f, 0.14f, 0.07f},
    {50, 1.0f, 0.09f, 0.032f},    {65, 1.0f, 0.07f, 0.017f},
    {100, 1.0f, 0.045f, 0.0075f}, {160, 1.0f, 0.027f, 0.0028f},
    {200, 1.0f, 0.022f, 0.0019f}, {325, 1.0f, 0.014f, 0.0007f},
    {600, 1.0f, 0.007f, 0.0002f}, {3250, 1.0f, 0.0014f, 0.000007f}};

Attenuation getAttenuationForDistance(float distance) {
    if (distance <= std::get<0>(attenuationTable.front())) {
        return {std::get<1>(attenuationTable.front()),
                std::get<2>(attenuationTable.front()),
                std::get<3>(attenuationTable.front())};
    }
    if (distance >= std::get<0>(attenuationTable.back())) {
        return {std::get<1>(attenuationTable.back()),
                std::get<2>(attenuationTable.back()),
                std::get<3>(attenuationTable.back())};
    }

    for (size_t i = 0; i < attenuationTable.size() - 1; ++i) {
        float d0 = std::get<0>(attenuationTable[i]);
        float d1 = std::get<0>(attenuationTable[i + 1]);

        if (distance >= d0 && distance <= d1) {
            float t = (distance - d0) / (d1 - d0);

            float constant = (1.0f - t) * std::get<1>(attenuationTable[i]) +
                             t * std::get<1>(attenuationTable[i + 1]);
            float linear = (1.0f - t) * std::get<2>(attenuationTable[i]) +
                           t * std::get<2>(attenuationTable[i + 1]);
            float quad = (1.0f - t) * std::get<3>(attenuationTable[i]) +
                         t * std::get<3>(attenuationTable[i + 1]);

            return {constant, linear, quad};
        }
    }

    return {1.0f, 0.045f, 0.0075f};
}

DirectionalLight::DirectionalLight(Position3d direction, Color color,
                                   Scene *scene)
    : Light(Position3d(0.0f, 0.0f, 0.0f), color, LightType::Directional, scene,
            15.f),
      direction(direction) {

    glGenFramebuffers(1, &this->depthMapFBO);

    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    glGenTextures(1, &this->depthMapID);
    glBindTexture(GL_TEXTURE_2D, this->depthMapID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           this->depthMapID, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Directional Light Framebuffer is not complete!"
                  << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectionalLight::storeDepthMap(std::vector<CoreObject *> &objects) {
    glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
    glViewport(0, 0, 1024, 1024);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDepthFunc(GL_LESS);

    glm::vec3 minBounds = glm::vec3(FLT_MAX);
    glm::vec3 maxBounds = glm::vec3(-FLT_MAX);

    for (auto &object : objects) {
        if (object == nullptr)
            continue;

        glm::vec3 objPos = glm::vec3(object->modelMatrix[3]);

        float padding = 2.0f;
        minBounds = glm::min(minBounds, objPos - glm::vec3(padding));
        maxBounds = glm::max(maxBounds, objPos + glm::vec3(padding));
    }

    glm::vec3 sceneCenter = (minBounds + maxBounds) * 0.5f;
    float sceneSize = glm::length(maxBounds - minBounds);

    if (sceneSize < 1.0f) {
        sceneSize = 10.0f;
    }

    float halfSize = sceneSize * 0.6f;
    glm::mat4 lightProjection = glm::ortho(-halfSize, halfSize, -halfSize,
                                           halfSize, 0.1f, sceneSize * 2.0f);

    glm::vec3 lightDir = glm::normalize(this->direction.toVec3());
    glm::vec3 lightPos = sceneCenter - lightDir * sceneSize;

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(lightDir, up)) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, up);
    this->lightSpaceMatrix = lightProjection * lightView;

    if (this->depthShader.has_value() == false) {
        this->depthShader = CoreShaderProgram(
            {CoreShader(DEPTH_VERT, CoreShaderType::Vertex),
             CoreShader(EMPTY_FRAG, CoreShaderType::Fragment)});
    }

    depthShader->use();
    depthShader->setMatrix4("uLightSpaceMatrix", lightSpaceMatrix);

    glDisable(GL_CULL_FACE);

    for (auto &object : objects) {
        if (object == nullptr) {
            std::cerr << "Error: Attempting to render a null object."
                      << std::endl;
            continue;
        }

        depthShader->setMatrix4("uModel", object->modelMatrix);
        glBindVertexArray(object->attributes.VAO);

        if (object->attributes.EBO.has_value()) {
            glDrawElements(GL_TRIANGLES, object->attributes.indices->size(),
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, object->vertices.size());
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
