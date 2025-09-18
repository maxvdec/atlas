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

std::vector<LayoutDescriptor> CoreVertex::getLayoutDescriptors() {
    return {
        {0, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
         offsetof(CoreVertex, position)},
        {1, 4, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
         offsetof(CoreVertex, color)},
    };
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

void CoreObject::attachVertices(const std::vector<CoreVertex> &newVertices) {
    vertices = newVertices;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
}

void CoreObject::attachIndices(const std::vector<Index> &newIndices) {
    indices = newIndices;
    glGenBuffers(1, &ebo);
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

    glBindVertexArray(vao);
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        return;
    }
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}
