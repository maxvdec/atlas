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
#include "atlas/window.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
        vertexData.push_back(vertex.color.a);

        vertexData.push_back(vertex.textCoords.width);
        vertexData.push_back(vertex.textCoords.height);
    }

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

    CoreShader vertexShader(MAIN_VERT, CoreShaderType::Vertex);
    this->vertexShader = vertexShader;

    CoreShader fragmentShader(AMBIENT_FRAG, CoreShaderType::Fragment);
    this->fragmentShader = fragmentShader;

    CoreShaderProgram shaderProgram(makeShaderList());
    this->program = shaderProgram;
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    this->attributes.VBO = VBO;
    this->attributes.VAO = VAO;

    const auto vertexData = makeVertexData();
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
                 vertexData.data(), GL_STATIC_DRAW);
    if (this->attributes.indices.has_value()) {
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     this->attributes.indices->size() * sizeof(unsigned int),
                     this->attributes.indices->data(), GL_STATIC_DRAW);
        this->attributes.EBO = EBO;
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                          (void *)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    this->program.value().use();

    if (this->visualizeTexture) {
        this->program.value().setBool("uUseTexture", true);
    } else {
        this->program.value().setBool("uUseTexture", false);
    }
    this->program.value().setInt("uTexture", 0);

    this->program.value().setMatrix4("uModel", this->modelMatrix);

    auto dispatcher = [](CoreObject *object) {
        glUseProgram(object->program.value().ID);

        float windowAspect =
            (float)Window::current_window->framebufferSize.width /
            (float)Window::current_window->framebufferSize.height;
        glm::vec2 aspectCorrection;

        if (windowAspect > 1.0f) {
            aspectCorrection = glm::vec2(1.0f / windowAspect, 1.0f);
        } else {
            aspectCorrection = glm::vec2(1.0f, windowAspect);
        }

        object->program.value().setVec2("uAspectCorrection", aspectCorrection);

        object->program.value().setInt("uTexture", 0);
        object->program.value().setMatrix4("uModel", object->modelMatrix);
        object->program.value().setMatrix4("uProjection",
                                           object->projectionMatrix);
        object->program.value().setMatrix4("uView", object->viewMatrix);
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
    this->setObjectAlpha(0.0f);
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
    updateProjectionType(this->projectionType);
    this->modelMatrix = glm::mat4(1.0f);
    this->viewMatrix = glm::mat4(1.0f);
}

void CoreObject::setVertexColor(int index, Color color) {
    if (index < 0 || index >= static_cast<int>(this->vertices.size())) {
        throw std::out_of_range("Index out of range for vertex colors");
    }
    this->vertices[index].color = color;
}

void CoreObject::setObjectAlpha(float alpha) {
    for (auto &vertex : this->vertices) {
        vertex.color.a = alpha;
    }
}

void CoreObject::translate(float x, float y, float z) {
    this->modelMatrix = glm::translate(this->modelMatrix, glm::vec3(x, y, z));
}

void CoreObject::rotate(float anglem, Axis axis) {
    switch (axis) {
    case Axis::X:
        this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(anglem),
                                        glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case Axis::Y:
        this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(anglem),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        break;
    case Axis::Z:
        this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(anglem),
                                        glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    default:
        throw std::invalid_argument("Invalid axis for rotation");
    }
}

void CoreObject::scale(float x, float y, float z) {
    this->modelMatrix = glm::scale(this->modelMatrix, glm::vec3(x, y, z));
}

void CoreShaderProgram::setMatrix4(const std::string &name,
                                   const glm::mat4 &matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(matrix));
}

void CoreShaderProgram::setVec2(const std::string &name,
                                const glm::vec2 &vector) const {
    glUniform2fv(glGetUniformLocation(this->ID, name.c_str()), 1,
                 glm::value_ptr(vector));
}

void CoreObject::updateProjectionType(ProjectionType type) {
    this->projectionType = type;
    if (type == ProjectionType::Orthographic) {
        this->projectionMatrix =
            glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        this->projectionMatrix = glm::perspective(
            glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    }
}
