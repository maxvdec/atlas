/*
 core_object.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object implementation and logic
 Copyright (c) 2025 maxvdec
*/

#include "atlas/object.h"
#include <glad/glad.h>

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

    if (indices.size() > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(Index),
                     indices.data(), GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(CoreVertex),
                          (void *)offsetof(CoreVertex, position));
    glEnableVertexAttribArray(0);

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
