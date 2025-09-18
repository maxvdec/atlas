/*
 core_object.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object implementation and logic
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/object.h"
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
             offsetof(CoreVertex, textureCoordinate)}};
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

void CoreObject::renderColorWithTexture() { onlyTexture = false; }

void CoreObject::attachTexture(const Texture &tex) { textures.push_back(tex); }

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

    if (!textures.empty()) {
        shaderProgram.setUniformBool("useTexture", true);
        shaderProgram.setUniformBool("onlyTexture", onlyTexture);

        int count = std::min((int)textures.size(), 16);
        shaderProgram.setUniform1i("textureCount", count);
        GLint units[16];
        for (int i = 0; i < count; i++)
            units[i] = i;

        glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                     count, units);

        for (int i = 0; i < count; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
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
