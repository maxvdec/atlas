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

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CoreVertex),
                 vertices.data(), GL_STATIC_DRAW);

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
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}
