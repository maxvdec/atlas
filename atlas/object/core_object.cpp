/*
 core_object.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object implementation and logic
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/window.h"
#include <algorithm>
#include <glad/glad.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::vector<LayoutDescriptor> CoreVertex::getLayoutDescriptors() {
    return {{0, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, position)},
            {1, 4, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, color)},
            {2, 2, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, textureCoordinate)},
            {3, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, normal)},
            {4, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, tangent)},
            {5, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
             offsetof(CoreVertex, bitangent)}};
}

CoreObject::CoreObject() : vbo(0), vao(0) {
    shaderProgram = ShaderProgram::defaultProgram();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, UINT32_MAX);

    id = dist(gen);
}

void CoreObject::attachProgram(const ShaderProgram &program) {
    shaderProgram = program;
    if (shaderProgram.programId == 0) {
        shaderProgram.compile();
    }
}

void CoreObject::createAndAttachProgram(VertexShader &vertexShader,
                                        FragmentShader &fragmentShader) {
    if (vertexShader.shaderId == 0) {
        vertexShader.compile();
    }

    if (fragmentShader.shaderId == 0) {
        fragmentShader.compile();
    }

    shaderProgram = ShaderProgram(vertexShader, fragmentShader);
    shaderProgram.compile();
}

void CoreObject::renderColorWithTexture() {
    useColor = true;
    useTexture = true;
}

void CoreObject::renderOnlyColor() {
    useColor = true;
    useTexture = false;
}

void CoreObject::renderOnlyTexture() {
    useColor = false;
    useTexture = true;
}

void CoreObject::attachTexture(const Texture &tex) {
    textures.push_back(tex);
    useTexture = true;
    useColor = false;
}

void CoreObject::setColor(const Color &color) {
    for (auto &vertex : vertices) {
        vertex.color = color;
    }
    useColor = true;
    useTexture = false;
    updateVertices();
}

void CoreObject::attachVertices(const std::vector<CoreVertex> &newVertices) {
    if (newVertices.empty()) {
        throw std::runtime_error("Cannot attach empty vertex array");
    }

    vertices = newVertices;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
}

void CoreObject::attachIndices(const std::vector<Index> &newIndices) {
    indices = newIndices;
    glGenBuffers(1, &ebo);
}

void CoreObject::setPosition(const Position3d &newPosition) {
    position = newPosition;

    if (hasPhysics && body != nullptr) {
        body->position = position;
    }

    updateModelMatrix();
}

void CoreObject::setRotation(const Rotation3d &newRotation) {
    rotation = newRotation;

    if (hasPhysics && body != nullptr) {
        body->orientation = rotation.toGlmQuat();
    }

    updateModelMatrix();
}

void CoreObject::setScale(const Scale3d &newScale) {
    scale = newScale;

    updateModelMatrix();
}

void CoreObject::move(const Position3d &delta) {
    setPosition(position + delta);
}

void CoreObject::rotate(const Rotation3d &delta) {
    setRotation(rotation + delta);
}

void CoreObject::lookAt(const Position3d &target, const Normal3d &up) {
    glm::vec3 pos = position.toGlm();
    glm::vec3 targetPos = target.toGlm();
    glm::vec3 upVec = up.toGlm();

    glm::vec3 forward = glm::normalize(targetPos - pos);

    glm::vec3 right = glm::normalize(glm::cross(forward, upVec));

    glm::vec3 realUp = glm::cross(right, forward);

    glm::mat3 rotMatrix;
    rotMatrix[0] = right;    // X-axis
    rotMatrix[1] = realUp;   // Y-axis
    rotMatrix[2] = -forward; // Z-axis (negative because OpenGL looks down -Z)

    float pitch, yaw, roll;

    pitch = glm::degrees(asin(glm::clamp(rotMatrix[2][1], -1.0f, 1.0f)));

    if (abs(cos(glm::radians(pitch))) > 0.00001f) {
        yaw = glm::degrees(atan2(-rotMatrix[2][0], rotMatrix[2][2]));
        roll = glm::degrees(atan2(-rotMatrix[0][1], rotMatrix[1][1]));
    } else {
        yaw = glm::degrees(atan2(rotMatrix[1][0], rotMatrix[0][0]));
        roll = 0.0f;
    }

    rotation = Rotation3d{roll, yaw, pitch};
    updateModelMatrix();
}

void CoreObject::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.roll)),
                    glm::vec3(0, 0, 1));
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.pitch)),
                    glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(
        rotation_matrix, glm::radians(float(rotation.yaw)), glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    model = translation_matrix * rotation_matrix * scale_matrix;
}

void CoreObject::initialize() {
    for (auto &component : components) {
        component->init();
    }
    if (vertices.empty()) {
        throw std::runtime_error("No vertices attached to the object");
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CoreVertex),
                 vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(Index),
                     indices.data(), GL_STATIC_DRAW);
    }

    std::vector<LayoutDescriptor> layoutDescriptors =
        CoreVertex::getLayoutDescriptors();

    std::vector<GLuint> activeLocations = shaderProgram.desiredAttributes;
    if (activeLocations.empty()) {
        for (const auto &attr : layoutDescriptors) {
            activeLocations.push_back(attr.layoutPos);
        }
    }

    for (const auto &attr : layoutDescriptors) {
        if (std::find(activeLocations.begin(), activeLocations.end(),
                      attr.layoutPos) != activeLocations.end()) {
            glEnableVertexAttribArray(attr.layoutPos);
            glVertexAttribPointer(attr.layoutPos, attr.size, attr.type,
                                  attr.normalized, attr.stride,
                                  (void *)attr.offset);
        }
    }

    if (!instances.empty()) {
        std::vector<glm::mat4> modelMatrices;
        for (auto &instance : instances) {
            instance.updateModelMatrix();
            modelMatrices.push_back(instance.getModelMatrix());
        }

        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4),
                     modelMatrices.data(), GL_STATIC_DRAW);

        glBindVertexArray(vao);
        std::size_t vec4Size = sizeof(glm::vec4);
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(6 + i);
            glVertexAttribPointer(6 + i, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(glm::mat4), (void *)(i * vec4Size));
            glVertexAttribDivisor(6 + i, 1);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CoreObject::render(float dt) {
    for (auto &component : components) {
        component->update(dt);
    }
    if (!isVisible) {
        return;
    }
    if (shaderProgram.programId == 0) {
        throw std::runtime_error("Shader program not compiled");
    }

    int currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (static_cast<Id>(currentProgram) != shaderProgram.programId) {
        glUseProgram(shaderProgram.programId);
    }

    shaderProgram.setUniformMat4f("model", model);
    shaderProgram.setUniformMat4f("view", view);
    shaderProgram.setUniformMat4f("projection", projection);

    shaderProgram.setUniform1i("useColor", useColor ? 1 : 0);
    shaderProgram.setUniform1i("useTexture", useTexture ? 1 : 0);

    int boundTextures = 0;
    int boundCubemaps = 0;

    if (!textures.empty() && useTexture &&
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Textures) !=
            shaderProgram.capabilities.end()) {
        int count = std::min((int)textures.size(), 10);
        shaderProgram.setUniform1i("textureCount", count);
        GLint units[10];
        for (int i = 0; i < count; i++)
            units[i] = i;

        for (int i = 0; i < count; i++) {
            std::string uniformName = "texture" + std::to_string(i + 1) + "";
            shaderProgram.setUniform1i(uniformName, i);
        }

        GLint cubeMapUnits[5];
        for (int i = 0; i < 5; i++)
            cubeMapUnits[i] = 10 + i;
        shaderProgram.setUniform1i("cubeMapCount", 5);
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1) + "";
            shaderProgram.setUniform1i(uniformName, i + 10);
        }

        GLint textureTypes[10];
        for (int i = 0; i < count; i++)
            textureTypes[i] = static_cast<int>(textures[i].type);
        for (int i = 0; i < count; i++) {
            std::string uniformName = "textureTypes[" + std::to_string(i) + "]";
            shaderProgram.setUniform1i(uniformName, textureTypes[i]);
        }

        for (int i = 0; i < count; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
            boundTextures++;
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Material) !=
        shaderProgram.capabilities.end()) {
        // Set material properties
        shaderProgram.setUniform3f("material.albedo", material.albedo.r,
                                   material.albedo.g, material.albedo.b);
        shaderProgram.setUniform1f("material.metallic", material.metallic);
        shaderProgram.setUniform1f("material.roughness", material.roughness);
        shaderProgram.setUniform1f("material.ao", material.ao);
        shaderProgram.setUniform1f("material.reflectivity",
                                   material.reflectivity);
    }

    const bool shaderSupportsIbl =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::IBL) != shaderProgram.capabilities.end();

    bool hasHdrEnvironment = std::any_of(
        textures.begin(), textures.end(), [](const Texture &texture) {
            return texture.type == TextureType::HDR;
        });

    const bool useIbl = shaderSupportsIbl && hasHdrEnvironment;
    shaderProgram.setUniformBool("useIBL", useIbl);

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Lighting) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();

        // Set ambient light
        Color ambientColor = scene->getAmbientColor();
        float ambientIntensity = scene->getAmbientIntensity();
        if (!useIbl && scene->isAutomaticAmbientEnabled()) {
            ambientColor = scene->getAutomaticAmbientColor();
            ambientIntensity = scene->getAutomaticAmbientIntensity();
        }
        shaderProgram.setUniform4f("ambientLight.color",
                                   static_cast<float>(ambientColor.r),
                                   static_cast<float>(ambientColor.g),
                                   static_cast<float>(ambientColor.b), 1.0f);
        shaderProgram.setUniform1f("ambientLight.intensity",
                                   ambientIntensity / 2.0f);

        // Set camera position
        shaderProgram.setUniform3f(
            "cameraPosition", window->getCamera()->position.x,
            window->getCamera()->position.y, window->getCamera()->position.z);

        // Send directional lights
        int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
        shaderProgram.setUniform1i("directionalLightCount", dirLightCount);

        for (int i = 0; i < dirLightCount; i++) {
            DirectionalLight *light = scene->directionalLights[i];
            std::string baseName =
                "directionalLights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".direction",
                                       light->direction.x, light->direction.y,
                                       light->direction.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);
        }

        // Send point lights

        int pointLightCount = std::min((int)scene->pointLights.size(), 256);
        shaderProgram.setUniform1i("pointLightCount", pointLightCount);

        for (int i = 0; i < pointLightCount; i++) {
            Light *light = scene->pointLights[i];
            std::string baseName = "pointLights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);

            PointLightConstants plc = light->calculateConstants();
            shaderProgram.setUniform1f(baseName + ".constant", plc.constant);
            shaderProgram.setUniform1f(baseName + ".linear", plc.linear);
            shaderProgram.setUniform1f(baseName + ".quadratic", plc.quadratic);
            shaderProgram.setUniform1f(baseName + ".radius", plc.radius);
        }

        // Send spotlights

        int spotlightCount = std::min((int)scene->spotlights.size(), 256);
        shaderProgram.setUniform1i("spotlightCount", spotlightCount);

        for (int i = 0; i < spotlightCount; i++) {
            Spotlight *light = scene->spotlights[i];
            std::string baseName = "spotlights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform3f(baseName + ".direction",
                                       light->direction.x, light->direction.y,
                                       light->direction.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);
            shaderProgram.setUniform1f(baseName + ".cutOff", light->cutOff);
            shaderProgram.setUniform1f(baseName + ".outerCutOff",
                                       light->outerCutoff);
        }

        // Send area lights

        int areaLightCount = std::min((int)scene->areaLights.size(), 256);
        shaderProgram.setUniform1i("areaLightCount", areaLightCount);

        for (int i = 0; i < areaLightCount; i++) {
            AreaLight *light = scene->areaLights[i];
            std::string baseName = "areaLights[" + std::to_string(i) + "]";

            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);

            shaderProgram.setUniform3f(baseName + ".right", light->right.x,
                                       light->right.y, light->right.z);

            shaderProgram.setUniform3f(baseName + ".up", light->up.x,
                                       light->up.y, light->up.z);

            shaderProgram.setUniform2f(baseName + ".size",
                                       static_cast<float>(light->size.width),
                                       static_cast<float>(light->size.height));

            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);

            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);

            shaderProgram.setUniform1f(baseName + ".angle", light->angle);
            shaderProgram.setUniform1i(baseName + ".castsBothSides",
                                       light->castsBothSides ? 1 : 0);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::LightDeferred) !=
        shaderProgram.capabilities.end()) {
        // Bind G-Buffer textures
        Window *window = Window::mainWindow;
        RenderTarget *gBuffer = window->gBuffer.get();
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gPosition.id);
        shaderProgram.setUniform1i("gPosition", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gNormal.id);
        shaderProgram.setUniform1i("gNormal", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gAlbedoSpec.id);
        shaderProgram.setUniform1i("gAlbedoSpec", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gMaterial.id);
        shaderProgram.setUniform1i("gMaterial", boundTextures);
        boundTextures++;
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Shadows) !=
        shaderProgram.capabilities.end()) {
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1);
            shaderProgram.setUniform1i(uniformName, i + 10);
        }
        // Bind shadow maps
        Scene *scene = Window::mainWindow->currentScene;

        int boundParameters = 0;

        // Cycle though directional lights
        for (auto light : scene->directionalLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundTextures);
            ShadowParams shadowParams = light->calculateLightSpaceMatrix(
                Window::mainWindow->renderables);
            shaderProgram.setUniformMat4f(baseName + ".lightView",
                                          shadowParams.lightView);
            shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                          shadowParams.lightProjection);
            shaderProgram.setUniform1f(baseName + ".bias", shadowParams.bias);
            shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

            boundParameters++;
            boundTextures++;
        }

        // Cycle though spotlights
        for (auto light : scene->spotlights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundTextures);
            std::tuple<glm::mat4, glm::mat4> lightSpace =
                light->calculateLightSpaceMatrix();
            shaderProgram.setUniformMat4f(baseName + ".lightView",
                                          std::get<0>(lightSpace));
            shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                          std::get<1>(lightSpace));
            shaderProgram.setUniform1f(baseName + ".bias", 0.005f);
            shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

            boundParameters++;
            boundTextures++;
        }

        for (auto light : scene->pointLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures + 6 >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + 10 + boundCubemaps);

            glBindTexture(GL_TEXTURE_CUBE_MAP,
                          light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundCubemaps);
            shaderProgram.setUniform1f(baseName + ".farPlane", light->distance);
            shaderProgram.setUniform3f(baseName + ".lightPos",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform1i(baseName + ".isPointLight", 1);

            boundParameters++;
            boundTextures += 6;
        }

        shaderProgram.setUniform1i("shadowParamCount", boundParameters);

        GLint units[16];
        for (int i = 0; i < boundTextures; i++)
            units[i] = i;

        glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                     boundTextures, units);
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::EnvironmentMapping) !=
        shaderProgram.capabilities.end()) {
        // Bind skybox
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        if (scene->skybox != nullptr) {
            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_CUBE_MAP, scene->skybox->cubemap.id);
            shaderProgram.setUniform1i("skybox", boundTextures);
            boundTextures++;
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Environment) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        shaderProgram.setUniform1f("environment.rimLightIntensity",
                                   scene->environment.rimLight.intensity);
        shaderProgram.setUniform3f("environment.rimLightColor",
                                   scene->environment.rimLight.color.r,
                                   scene->environment.rimLight.color.g,
                                   scene->environment.rimLight.color.b);
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Instances) !=
            shaderProgram.capabilities.end() &&
        !instances.empty()) {
        if (this->instances != this->savedInstances) {
            updateInstances();
            this->savedInstances = this->instances;
        }
        // Update instance buffer data
        glBindVertexArray(vao);
        shaderProgram.setUniform1i("isInstanced", 1);

        if (!indices.empty()) {
            glDrawElementsInstanced(GL_TRIANGLES, indices.size(),
                                    GL_UNSIGNED_INT, 0, instances.size());
            glBindVertexArray(0);
            return;
        }
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(),
                              instances.size());
        glBindVertexArray(0);
        return;
    }

    glBindVertexArray(vao);
    shaderProgram.setUniform1i("isInstanced", 0);
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        return;
    }
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

void CoreObject::setViewMatrix(const glm::mat4 &view) {
    this->view = view;
    for (auto &component : components) {
        component->setViewMatrix(view);
    }
}

void CoreObject::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
    for (auto &component : components) {
        component->setProjectionMatrix(projection);
    }
}

CoreObject CoreObject::clone() const {
    CoreObject newObject = *this;

    glGenVertexArrays(1, &newObject.vao);
    glGenBuffers(1, &newObject.vbo);
    glGenBuffers(1, &newObject.ebo);

    newObject.initialize();

    return newObject;
}

void CoreObject::updateVertices() {
    if (vbo == 0 || vertices.empty()) {
        throw std::runtime_error("Cannot update vertices: VBO not "
                                 "initialized or empty vertex list");
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(CoreVertex),
                    vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CoreObject::update(Window &window) {
    if (!hasPhysics)
        return;

    this->body->update(window);

    this->position = this->body->position;
    this->rotation = Rotation3d::fromGlmQuat(this->body->orientation);
    updateModelMatrix();
}

void CoreObject::setupPhysics(Body body) {
    this->body = std::make_shared<Body>(body);
    this->body->position = this->position;
    this->body->orientation = this->rotation.toGlmQuat();
    this->hasPhysics = true;
}

void CoreObject::makeEmissive(Scene *scene, Color emissionColor,
                              float intensity) {
    this->initialize();
    if (light != nullptr) {
        throw std::runtime_error("Object is already emissive");
    }
    light = std::make_shared<Light>();
    light->color = emissionColor;
    light->shineColor = emissionColor;
    light->position = this->position;
    light->distance = 10.0f;
    light->doesCastShadows = false;
    this->useDeferredRendering = false;

    for (auto &vertex : vertices) {
        vertex.color = emissionColor * intensity;
    }
    updateVertices();
    useColor = true;
    useTexture = false;

    this->renderOnlyColor();
    this->attachProgram(ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Color, AtlasFragmentShader::Color));

    scene->addLight(light.get());
}

void CoreObject::updateInstances() {
    if (instances.empty()) {
        return;
    }

    if (this->instanceVBO == 0) {
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4),
                     nullptr, GL_DYNAMIC_DRAW);
        glBindVertexArray(vao);
        std::size_t vec4Size = sizeof(glm::vec4);
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(6 + i);
            glVertexAttribPointer(6 + i, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(glm::mat4), (void *)(i * vec4Size));
            glVertexAttribDivisor(6 + i, 1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(instances.size());

    for (auto &instance : instances) {
        instance.updateModelMatrix();
        modelMatrices.push_back(instance.getModelMatrix());
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(glm::mat4),
                    modelMatrices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Instance::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.roll)),
                    glm::vec3(0, 0, 1));
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.pitch)),
                    glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(
        rotation_matrix, glm::radians(float(rotation.yaw)), glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    this->model = translation_matrix * rotation_matrix * scale_matrix;
}

void Instance::move(const Position3d &deltaPosition) {
    setPosition(position + deltaPosition);
}

void Instance::setPosition(const Position3d &newPosition) {
    position = newPosition;
    updateModelMatrix();
}

void Instance::setRotation(const Rotation3d &newRotation) {
    rotation = newRotation;
    updateModelMatrix();
}

void Instance::rotate(const Rotation3d &deltaRotation) {
    setRotation(rotation + deltaRotation);
}

void Instance::setScale(const Scale3d &newScale) {
    scale = newScale;
    updateModelMatrix();
}

void Instance::scaleBy(const Scale3d &deltaScale) {
    setScale(Scale3d(scale.x * deltaScale.x, scale.y * deltaScale.y,
                     scale.z * deltaScale.z));
}
