/*
 shader.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader utilities and functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/core/default_shaders.h"
#include "atlas/units.h"
#include <glad/glad.h>
#include <string>

VertexShader VertexShader::fromDefaultShader(AtlasVertexShader shader) {
    switch (shader) {
    case AtlasVertexShader::Debug:
        return VertexShader::fromSource(DEBUG_VERT);
    default:
        throw std::runtime_error("Unknown default vertex shader");
    }
}

VertexShader VertexShader::fromSource(const char *source) {
    VertexShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void VertexShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Vertex shader already compiled");
    }

    shaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Vertex shader compilation failed: ") + infoLog);
    }
}

FragmentShader FragmentShader::fromDefaultShader(AtlasFragmentShader shader) {
    switch (shader) {
    case AtlasFragmentShader::Debug:
        return FragmentShader::fromSource(DEBUG_FRAG);
    default:
        throw std::runtime_error("Unknown default fragment shader");
    }
}

FragmentShader FragmentShader::fromSource(const char *source) {
    FragmentShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void FragmentShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Fragment shader already compiled");
    }

    shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Fragment shader compilation failed: ") + infoLog);
    }
}

void ShaderProgram::compile() {
    if (programId != 0) {
        throw std::runtime_error("Shader program already compiled");
    }

    if (vertexShader.shaderId == 0) {
        throw std::runtime_error("Vertex shader not compiled");
    }

    if (fragmentShader.shaderId == 0) {
        throw std::runtime_error("Fragment shader not compiled");
    }

    programId = glCreateProgram();
    glAttachShader(programId, vertexShader.shaderId);
    glAttachShader(programId, fragmentShader.shaderId);
    glLinkProgram(programId);

    GLint success;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Shader program linking failed: ") + infoLog);
    }

    glDeleteShader(vertexShader.shaderId);
    glDeleteShader(fragmentShader.shaderId);
}

ShaderProgram ShaderProgram::defaultProgram() {
    ShaderProgram program;
    program.vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::DEFAULT_VERT_SHADER);
    program.fragmentShader = FragmentShader::fromDefaultShader(
        AtlasFragmentShader::DEFAULT_FRAG_SHADER);
    program.programId = 0;
    program.vertexShader.compile();
    program.fragmentShader.compile();
    program.compile();
    return program;
}
