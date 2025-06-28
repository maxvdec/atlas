/*
 coreObject.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object definitions and rendering code
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include <OpenGL/gl.h>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenGL/glext.h>
#endif

std::vector<float> CoreObject::makeVertexData() const {
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 3);

    for (const auto &vertex : vertices) {
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);
    }

    return vertexData;
}

CoreShader::CoreShader(std::string code, CoreShaderType type) {
    unsigned int shader;
    switch (type) {
    case CoreShaderType::Vertex:
        shader = glCreateShader(GL_VERTEX_SHADER);
        break;
    case CoreShaderType::Fragment:
        shader = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    default:
        throw std::runtime_error("Unsupported shader type");
    }

    const char *source = code.c_str();

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::string errorMessage = "Shader compilation failed: ";
        errorMessage += infoLog;
        glDeleteShader(shader);
        throw std::runtime_error(errorMessage);
    }

    this->ID = shader;
}

CoreShaderProgram::CoreShaderProgram(const std::vector<CoreShader> &shaders) {
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    for (const auto &shader : shaders) {
        glAttachShader(shaderProgram, shader.ID);
    }

    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::string errorMessage = "Shader program linking failed: ";
        errorMessage += infoLog;
        glDeleteProgram(shaderProgram);
        throw std::runtime_error(errorMessage);
    }

    for (const auto &shader : shaders) {
        glDeleteShader(shader.ID);
    }

    this->ID = shaderProgram;
}

void CoreShaderProgram::use() const { glUseProgram(this->ID); }

std::vector<CoreShader> CoreObject::makeShaderList() const {
    std::vector<CoreShader> shaderList;
    shaderList.push_back(this->vertexShader);
    shaderList.push_back(this->fragmentShader);
    for (const auto &shader : this->shaders) {
        shaderList.push_back(shader);
    }
    return shaderList;
}

void CoreObject::initialize() {
    unsigned int VAO;
#ifdef __APPLE__
    glGenVertexArraysAPPLE(1, &VAO);
    glBindVertexArrayAPPLE(VAO);
#else
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
#endif
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    this->attributes = {VBO, 0}; // VAO will be set later

    const auto vertexData = makeVertexData();
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData.data(),
                 GL_STATIC_DRAW);

    CoreShader vertexShader(NORMAL_VERT, CoreShaderType::Vertex);
    this->vertexShader = vertexShader;

    CoreShader fragmentShader(NORMAL_FRAG, CoreShaderType::Fragment);
    this->fragmentShader = fragmentShader;

    CoreShaderProgram shaderProgram(makeShaderList());
    this->program = shaderProgram;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    this->program.use();
}
