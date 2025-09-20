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
#include "atlas/scene.h"
#include "atlas/window.h"
#include <algorithm>
#include <glad/glad.h>
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
             offsetof(CoreVertex, normal)}};
}

CoreObject::CoreObject() : vbo(0), vao(0) {
    shaderProgram = ShaderProgram::defaultProgram();
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

    updateModelMatrix();
}

void CoreObject::setRotation(const Rotation3d &newRotation) {
    rotation = newRotation;

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

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CoreObject::render() {
    if (!isVisible)
        return;
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

    if (!textures.empty() && useTexture &&
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Textures) !=
            shaderProgram.capabilities.end()) {
        int count = std::min((int)textures.size(), 16);
        shaderProgram.setUniform1i("textureCount", count);
        GLint units[16];
        for (int i = 0; i < count; i++)
            units[i] = i;

        glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                     count, units);

        GLint textureTypes[16];
        for (int i = 0; i < count; i++)
            textureTypes[i] = static_cast<int>(textures[i].type);
        glUniform1iv(
            glGetUniformLocation(shaderProgram.programId, "textureTypes"),
            count, textureTypes);

        for (int i = 0; i < count; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Lighting) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();

        // Set ambient light
        shaderProgram.setUniform4f(
            "ambientLight.color", scene->ambientLight.color.r,
            scene->ambientLight.color.g, scene->ambientLight.color.b, 1.0f);
        shaderProgram.setUniform1f("ambientLight.intensity",
                                   scene->ambientLight.intensity);

        // Set camera position
        shaderProgram.setUniform3f(
            "cameraPosition", window->getCamera()->position.x,
            window->getCamera()->position.y, window->getCamera()->position.z);

        // Set material properties
        shaderProgram.setUniform3f("material.ambient", material.ambient.r,
                                   material.ambient.g, material.ambient.b);
        shaderProgram.setUniform3f("material.diffuse", material.diffuse.r,
                                   material.diffuse.g, material.diffuse.b);
        shaderProgram.setUniform3f("material.specular", material.specular.r,
                                   material.specular.g, material.specular.b);
        shaderProgram.setUniform1f("material.shininess", material.shininess);

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
    }

    glBindVertexArray(vao);
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        return;
    }
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

void CoreObject::setViewMatrix(const glm::mat4 &view) { this->view = view; }

void CoreObject::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
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
        throw std::runtime_error(
            "Cannot update vertices: VBO not initialized or empty vertex list");
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(CoreVertex),
                    vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
