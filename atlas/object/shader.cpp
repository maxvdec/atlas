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
    VertexShader vertexShader;
    switch (shader) {
    case AtlasVertexShader::Debug: {
        vertexShader = VertexShader::fromSource(DEBUG_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        break;
    }
    case AtlasVertexShader::Color: {
        vertexShader = VertexShader::fromSource(COLOR_VERT);
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {};
        break;
    }
    case AtlasVertexShader::Main: {
        vertexShader = VertexShader::fromSource(MAIN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3};
        vertexShader.capabilities = {ShaderCapability::Lighting,
                                     ShaderCapability::Textures};
        break;
    }
    case AtlasVertexShader::Texture: {
        vertexShader = VertexShader::fromSource(TEXTURE_VERT);
        vertexShader.desiredAttributes = {0, 1, 2};
        vertexShader.capabilities = {ShaderCapability::Textures};
        break;
    }
    case AtlasVertexShader::Fullscreen: {
        vertexShader = VertexShader::fromSource(FULLSCREEN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3};
        vertexShader.capabilities = {ShaderCapability::Textures};
        break;
    }
    default:
        throw std::runtime_error("Unknown default vertex shader");
    }

    return vertexShader;
}

VertexShader VertexShader::fromSource(const char *source) {
    VertexShader shader;
    shader.source = source;
    shader.shaderId = 0;
    shader.desiredAttributes = {};
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
    case AtlasFragmentShader::Color:
        return FragmentShader::fromSource(COLOR_FRAG);
    case AtlasFragmentShader::Main:
        return FragmentShader::fromSource(MAIN_FRAG);
    case AtlasFragmentShader::Texture:
        return FragmentShader::fromSource(TEXTURE_FRAG);
    case AtlasFragmentShader::Fullscreen:
        return FragmentShader::fromSource(FULLSCREEN_FRAG);
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

    desiredAttributes = vertexShader.desiredAttributes;
    capabilities = vertexShader.capabilities;

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
    program.desiredAttributes = program.vertexShader.desiredAttributes;
    program.vertexShader.compile();
    program.fragmentShader.compile();
    program.compile();
    return program;
}

void ShaderProgram::setUniform4f(std::string name, float v0, float v1, float v2,
                                 float v3) {
    glUniform4f(glGetUniformLocation(programId, name.c_str()), v0, v1, v2, v3);
}

void ShaderProgram::setUniform3f(std::string name, float v0, float v1,
                                 float v2) {
    glUniform3f(glGetUniformLocation(programId, name.c_str()), v0, v1, v2);
}

void ShaderProgram::setUniform2f(std::string name, float v0, float v1) {
    glUniform2f(glGetUniformLocation(programId, name.c_str()), v0, v1);
}

void ShaderProgram::setUniform1f(std::string name, float v0) {
    glUniform1f(glGetUniformLocation(programId, name.c_str()), v0);
}

void ShaderProgram::setUniformMat4f(std::string name, const glm::mat4 &matrix) {
    glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1,
                       GL_FALSE, &matrix[0][0]);
}

void ShaderProgram::setUniform1i(std::string name, int v0) {
    glUniform1i(glGetUniformLocation(programId, name.c_str()), v0);
}

void ShaderProgram::setUniformBool(std::string name, bool value) {
    glUniform1i(glGetUniformLocation(programId, name.c_str()), (int)value);
}
