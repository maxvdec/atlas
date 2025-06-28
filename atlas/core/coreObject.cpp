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
#include <glad/glad.h>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

std::vector<float> CoreObject::makeVertexData() const {
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 6);

    for (const auto &vertex : vertices) {
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);

        vertexData.push_back(vertex.color.r);
        vertexData.push_back(vertex.color.g);
        vertexData.push_back(vertex.color.b);

        vertexData.push_back(vertex.textCoords.width);
        vertexData.push_back(vertex.textCoords.height);
        std::cout << "Vertex: (" << vertex.x << ", " << vertex.y << ", "
                  << vertex.z << ") Color: (" << vertex.color.r << ", "
                  << vertex.color.g << ", " << vertex.color.b << ")\n";
    }

    std::cout << "Total vertices: " << vertices.size() << "\n";
    return vertexData;
}

void CoreObject::provideTextureCoords(std::vector<Size2d> textureCoords) {
    if (textureCoords.size() != vertices.size()) {
        throw std::runtime_error(
            "Texture coordinates size must match vertices size");
    }
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].textCoords = textureCoords[i];
    }
}

void CoreObject::provideColors(std::vector<Color> colors) {
    if (colors.size() != vertices.size()) {
        throw std::runtime_error("Colors size must match vertices size");
    }
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].color = colors[i];
    }
}

void CoreObject::provideVertexData(std::vector<CoreVertex> vertices) {
    this->vertices = std::move(vertices);
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

void CoreShaderProgram::setFloat(const std::string &name, float val) const {
    glUniform1f(glGetUniformLocation(this->ID, name.c_str()), val);
}

void CoreShaderProgram::setInt(const std::string &name, int val) const {
    glUniform1i(glGetUniformLocation(this->ID, name.c_str()), val);
}

void CoreShaderProgram::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
}

void CoreShaderProgram::use() const { glUseProgram(this->ID); }

std::vector<CoreShader> CoreObject::makeShaderList() const {
    std::vector<CoreShader> shaderList;
    shaderList.push_back(this->vertexShader.value());
    shaderList.push_back(this->fragmentShader.value());
    for (const auto &shader : this->shaders) {
        shaderList.push_back(shader);
    }
    return shaderList;
}

void CoreObject::initialize() {
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    if (this->attributes.indices.has_value()) {
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     this->attributes.indices->size() * sizeof(unsigned int),
                     this->attributes.indices->data(), GL_STATIC_DRAW);
        this->attributes.EBO = EBO;
    }

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    this->attributes.VBO = VBO;
    this->attributes.VAO = VAO;

    const auto vertexData = makeVertexData();
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
                 vertexData.data(), GL_STATIC_DRAW);

    CoreShader vertexShader(NORMAL_VERT, CoreShaderType::Vertex);
    this->vertexShader = vertexShader;

    CoreShader fragmentShader(NORMAL_FRAG, CoreShaderType::Fragment);
    this->fragmentShader = fragmentShader;

    CoreShaderProgram shaderProgram(makeShaderList());
    this->program = shaderProgram;

    if (this->visualizeTexture) {
        this->program.value().setBool("uUseTexture", true);
    } else {
        this->program.value().setBool("uUseTexture", false);
    }
    this->program.value().setInt("uTexture", 0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    this->program.value().use();

    auto dispatcher = [](CoreObject *object) {
        glUseProgram(object->program.value().ID);
        object->program.value().setInt("uTexture", 0);
        if (object->visualizeTexture) {
            object->program.value().setBool("uUseTexture", true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, object->texture.ID);
        } else {
            object->program.value().setBool("uUseTexture", false);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glBindVertexArray(object->attributes.VAO);
        if (object->attributes.EBO.has_value()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                         object->attributes.EBO.value());
            glDrawElements(GL_TRIANGLES, object->attributes.elementCount,
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0,
                         static_cast<GLsizei>(object->vertices.size()));
        }
    };

    Renderer::instance().registerObject(this, dispatcher);
}

void CoreObject::setTexture(Texture texture) {
    this->texture = std::move(texture);
    this->visualizeTexture = true;
}

void CoreObject::enableTexturing() {
    this->visualizeTexture = true;
    if (!this->program.has_value()) {
        throw std::runtime_error(
            "Shader program not initialized, Do it before enabling texturing.");
    }
    this->program.value().setBool("uUseTexture", true);
}

void CoreObject::disableTexturing() {
    this->visualizeTexture = false;
    if (!this->program.has_value()) {
        throw std::runtime_error("Shader program not initialized, Do it before "
                                 "disabling texturing.");
    }
    this->program.value().setBool("uUseTexture", false);
}

void CoreObject::provideIndexedDrawing(std::vector<unsigned int> indices) {
    this->attributes.indices = std::move(indices);
    this->attributes.elementCount =
        static_cast<unsigned int>(this->attributes.indices->size());
}

CoreObject::CoreObject(std::vector<CoreVertex> vertices) {
    this->vertices = std::move(vertices);
    this->vertexShader = std::nullopt;
    this->fragmentShader = std::nullopt;
    this->program = std::nullopt;
    this->texture = Texture();
}
