//
// fluid.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Fluid implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "hydra/fluid.h"
#include "atlas/texture.h"
#include "atlas/window.h"

#include <cstddef>
#include <stdexcept>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

Fluid::Fluid() : GameObject() {
    buildPlaneGeometry();
    updateModelMatrix();
}

Fluid::~Fluid() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
    }
}

void Fluid::create(Size2d extent, Color color) {
    this->color = color;
    setExtent(extent);
    fluidShader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Fluid,
                                                    AtlasFragmentShader::Fluid);
}

void Fluid::initialize() {
    if (fluidShader.programId == 0) {
        throw std::runtime_error(
            "Fluid shader not initialized. Call create() before initialize().");
    }
    if (isInitialized) {
        return;
    }

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(FluidVertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(FluidVertex),
        reinterpret_cast<void *>(offsetof(FluidVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(FluidVertex),
        reinterpret_cast<void *>(offsetof(FluidVertex, texCoord)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE, sizeof(FluidVertex),
        reinterpret_cast<void *>(offsetof(FluidVertex, normal)));

    glBindVertexArray(0);

    isInitialized = true;
}

void Fluid::render(float dt) {
    if (!isInitialized) {
        initialize();
    }

    glDisable(GL_CULL_FACE);

    glUseProgram(fluidShader.programId);
    fluidShader.setUniformMat4f("model", modelMatrix);
    fluidShader.setUniformMat4f("view", viewMatrix);
    fluidShader.setUniformMat4f("projection", projectionMatrix);
    fluidShader.setUniform4f("waterColor", color.r, color.g, color.b, color.a);

    RenderTarget *target = Window::mainWindow->currentRenderTarget;
    if (target == nullptr) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target->texture.id);
    fluidShader.setUniform1i("sceneTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, target->depthTexture.id);
    fluidShader.setUniform1i("sceneDepth", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Window::mainWindow->getGBuffer()->gNormal.id);
    fluidShader.setUniform1i("normalMap", 2);

    fluidShader.setUniform3f("cameraPos",
                             Window::mainWindow->getCamera()->position.x,
                             Window::mainWindow->getCamera()->position.y,
                             Window::mainWindow->getCamera()->position.z);

    fluidShader.setUniform3f("waterNormal", 0.0f, 1.0f, 0.0f);
    fluidShader.setUniform1f("time", dt);
    fluidShader.setUniform1f("refractionStrength", 0.5f);
    fluidShader.setUniform1f("reflectionStrength", 0.5f);
    fluidShader.setUniform1f("depthFade", 0.1f);

    fluidShader.setUniformMat4f("invProjection",
                                glm::inverse(projectionMatrix));
    fluidShader.setUniformMat4f("invView", glm::inverse(viewMatrix));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
}

void Fluid::update(Window &window) { (void)window; }

void Fluid::setViewMatrix(const glm::mat4 &view) { viewMatrix = view; }

void Fluid::setProjectionMatrix(const glm::mat4 &projection) {
    projectionMatrix = projection;
}

void Fluid::move(const Position3d &delta) {
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
    updateModelMatrix();
}

void Fluid::setPosition(const Position3d &pos) {
    position = pos;
    updateModelMatrix();
}

void Fluid::setRotation(const Rotation3d &rot) {
    rotation = rot;
    updateModelMatrix();
}

void Fluid::rotate(const Rotation3d &delta) { setRotation(rotation + delta); }

void Fluid::setScale(const Scale3d &newScale) {
    scale = newScale;
    updateModelMatrix();
}

void Fluid::setExtent(const Size2d &ext) {
    extent = ext;
    extentScale = {ext.width, 1.0, ext.height};
    updateModelMatrix();
}

void Fluid::setWaterColor(const Color &newColor) { color = newColor; }

void Fluid::buildPlaneGeometry() {
    vertices = {
        FluidVertex{{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        FluidVertex{{0.5f, 0.0f, -0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        FluidVertex{{0.5f, 0.0f, 0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        FluidVertex{{-0.5f, 0.0f, 0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}};

    indices = {0, 1, 2, 0, 2, 3};
}

glm::vec3 Fluid::getFinalScale() const {
    double sx = extentScale.x * scale.x;
    double sy = extentScale.y * scale.y;
    double sz = extentScale.z * scale.z;
    return glm::vec3(static_cast<float>(sx), static_cast<float>(sy),
                     static_cast<float>(sz));
}

void Fluid::updateModelMatrix() {
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), getFinalScale());

    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix = glm::rotate(
        rotationMatrix, glm::radians(static_cast<float>(rotation.roll)),
        glm::vec3(0.0f, 0.0f, 1.0f));
    rotationMatrix = glm::rotate(
        rotationMatrix, glm::radians(static_cast<float>(rotation.pitch)),
        glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix,
                                 glm::radians(static_cast<float>(rotation.yaw)),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 translationMatrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
}