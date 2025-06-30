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
#include "atlas/light.hpp"
#include "atlas/texture.hpp"
#include "atlas/window.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

std::vector<float> CoreObject::makeVertexData() const {
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 12);

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

        vertexData.push_back(vertex.normal.width);
        vertexData.push_back(vertex.normal.height);
        vertexData.push_back(vertex.normal.depth);
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
    if (symbolExists(name)) {
        glUniform1f(glGetUniformLocation(this->ID, name.c_str()), val);
    } else {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
    }
}

void CoreShaderProgram::setInt(const std::string &name, int val) const {
    if (symbolExists(name)) {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), val);
    } else {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
    }
}

void CoreShaderProgram::setBool(const std::string &name, bool value) const {
    if (symbolExists(name)) {
        glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
    } else {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
    }
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
    if (!this->vertexShader.has_value()) {
        CoreShader vertexShader(MAIN_VERT, CoreShaderType::Vertex);
        this->vertexShader = vertexShader;
    }

    if (!this->fragmentShader.has_value()) {
        CoreShader fragmentShader(AMBIENT_FRAG, CoreShaderType::Fragment);
        this->fragmentShader = fragmentShader;
    }

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

    const int stride = 12 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);

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
        if (object->program.has_value() == false) {
            std::cout << "Skipping corrupted object with ID: " << object->id
                      << std::endl;
            return;
        }
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

        object->program.value().setMatrix4("uModel", object->modelMatrix);
        object->program.value().setMatrix4("uProjection",
                                           object->projectionMatrix);
        object->program.value().setMatrix4("uView", object->viewMatrix);
        if (object->program.value().symbolExists("uAmbientColor")) {
            object->program.value().setVec3(
                "uAmbientColor", Window::current_window->ambientColor.toVec3());
        }
        const auto lights = Window::current_window->currentScene->lights;
        if (object->program.value().symbolExists("uLights[0].position") &&
            lights.size() > 0) {

            int lightCount = std::min((int)lights.size(), MAX_LIGHTS);
            object->program.value().setInt("uLightCount", lightCount);

            for (int i = 0; i < lightCount; ++i) {
                const auto &light = lights[i];
                std::string lightBase = "uLights[" + std::to_string(i) + "]";

                object->program.value().setVec3(lightBase + ".position",
                                                light->position.toVec3());
                object->program.value().setVec3(lightBase + ".color",
                                                light->color.toVec3());
                object->program.value().setFloat(lightBase + ".intensity",
                                                 light->intensity);
                object->program.value().setVec3(
                    lightBase + ".specular", light->material.specular.toVec3());
                object->program.value().setVec3(lightBase + ".ambient",
                                                light->ambientColor.toVec3());
                object->program.value().setVec3(
                    lightBase + ".diffuse", light->material.diffuse.toVec3());

                object->program.value().setBool(lightBase + ".isDirectional",
                                                false);
                object->program.value().setBool(lightBase + ".isPointLight",
                                                false);
                object->program.value().setBool(lightBase + ".isSpotLight",
                                                false);

                if (light->type == LightType::Directional) {
                    const auto &directionalLight =
                        static_cast<DirectionalLight *>(light);
                    object->program.value().setBool(
                        lightBase + ".isDirectional", true);
                    object->program.value().setVec3(
                        lightBase + ".directionalLight.direction",
                        directionalLight->direction.toVec3());
                } else if (light->type == LightType::Point) {
                    const auto &pointLight = static_cast<PointLight *>(light);
                    object->program.value().setBool(lightBase + ".isPointLight",
                                                    true);
                    object->program.value().setVec3(
                        lightBase + ".position", pointLight->position.toVec3());

                    object->program.value().setFloat(
                        lightBase + ".pointLight.constant",
                        pointLight->attenuation.constant);
                    object->program.value().setFloat(
                        lightBase + ".pointLight.linear",
                        pointLight->attenuation.linear);
                    object->program.value().setFloat(
                        lightBase + ".pointLight.quadratic",
                        pointLight->attenuation.quadratic);

                } else if (light->type == LightType::SpotLight) {
                    const auto &spotLight = static_cast<SpotLight *>(light);
                    object->program.value().setBool(lightBase + ".isSpotLight",
                                                    true);
                    object->program.value().setBool(lightBase + ".isPointLight",
                                                    true);

                    object->program.value().setVec3(
                        lightBase + ".position", spotLight->position.toVec3());
                    object->program.value().setVec3(
                        lightBase + ".spotLight.direction",
                        spotLight->direction.toVec3());
                    object->program.value().setFloat(
                        lightBase + ".spotLight.cutOff", spotLight->cutOff);
                    object->program.value().setFloat(
                        lightBase + ".spotLight.outerCutOff",
                        spotLight->outerCutOff);

                    object->program.value().setFloat(
                        lightBase + ".pointLight.constant",
                        spotLight->attenuation.constant);
                    object->program.value().setFloat(
                        lightBase + ".pointLight.linear",
                        spotLight->attenuation.linear);
                    object->program.value().setFloat(
                        lightBase + ".pointLight.quadratic",
                        spotLight->attenuation.quadratic);
                }
            }

            object->program.value().setFloat("uMaterial.shininess",
                                             object->material.shininess);
            object->program.value().setVec3("uMaterial.specular",
                                            object->material.specular.toVec3());
            object->program.value().setVec3("uMaterial.diffuse",
                                            object->material.diffuse.toVec3());
        }
        if (object->program.value().symbolExists("uCameraPos")) {
            if (Window::current_window->mainCam != nullptr) {
                object->program.value().setVec3(
                    "uCameraPos",
                    Window::current_window->mainCam->position.toVec3());
            } else {
                object->program.value().setVec3("uCameraPos", glm::vec3(0.0f));
            }
        }
        if (object->visualizeTexture) {
            int specularMaps = 0;
            int diffuseMaps = 0;
            object->program.value().setBool("uUseTexture", true);

            for (int i = 0; i < object->textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, object->textures[i].ID);
                const auto &texture = object->textures[i];
                if (texture.type == TextureType::Specular) {
                    specularMaps++;

                    object->program.value().setInt(
                        "uMaterial.specularMap" + std::to_string(specularMaps),
                        i);
                    object->program.value().setBool("uMaterial.useSpecularMap",
                                                    true);
                } else if (texture.type == TextureType::Color) {
                    diffuseMaps++;
                    object->program.value().setInt(
                        "uTexture" + std::to_string(diffuseMaps), i);
                } else {
                    std::cerr
                        << "Unknown texture type for texture ID: " << texture.ID
                        << std::endl;
                }
            }
            if (specularMaps == 0) {
                object->program.value().setInt("uMaterial.specularMap", 0);
                object->program.value().setBool("uMaterial.useSpecularMap",
                                                false);
            }
            object->program.value().setInt("uTextureCount", diffuseMaps);
            object->program.value().setInt("uMaterial.specularMapCount",
                                           diffuseMaps);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, getDefaultTexture());
            object->program.value().setBool("uUseTexture", false);
        }
        glBindVertexArray(object->attributes.VAO);
        if (object->attributes.EBO.has_value()) {
            glDrawElements(GL_TRIANGLES, object->attributes.elementCount,
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0,
                         static_cast<GLsizei>(object->vertices.size()));
        }
    };

    Renderer::instance().registerObject(this, dispatcher);
}

void CoreObject::addTexture(Texture texture) {
    this->textures.push_back(std::move(texture));
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
    this->textures = {};
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
    if (!symbolExists(name)) {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
        return;
    }
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(matrix));
}

void CoreShaderProgram::setVec2(const std::string &name,
                                const glm::vec2 &vector) const {
    if (!symbolExists(name)) {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
        return;
    }
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

void CoreShaderProgram::setVec3(const std::string &name,
                                const glm::vec3 &vector) const {
    if (!symbolExists(name)) {
        std::cerr << "Warning: Uniform '" << name
                  << "' does not exist in shader program." << std::endl;
        return;
    }
    glUniform3fv(glGetUniformLocation(this->ID, name.c_str()), 1,
                 glm::value_ptr(vector));
}

GLuint defaultTexture = 0;

GLuint getDefaultTexture() {
    if (defaultTexture == 0) {
        glGenTextures(1, &defaultTexture);
        glBindTexture(GL_TEXTURE_2D, defaultTexture);
        unsigned char whitePixel[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    return defaultTexture;
}

void CoreObject::provideNormals(std::vector<Size3d> normals) {
    if (normals.size() != vertices.size()) {
        throw std::runtime_error("Normals size must match vertices size");
    }
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].normal = normals[i];
    }
}

CoreObject CoreObject::copy() {
    CoreObject copy;
    copy.vertices = this->vertices;
    copy.vertexShader = this->vertexShader;
    copy.fragmentShader = this->fragmentShader;
    copy.program = this->program;
    copy.attributes = this->attributes;
    copy.textures = this->textures;
    copy.visualizeTexture = this->visualizeTexture;
    copy.modelMatrix = this->modelMatrix;
    copy.viewMatrix = this->viewMatrix;
    copy.projectionMatrix = this->projectionMatrix;
    copy.projectionType = this->projectionType;
    return copy;
}
