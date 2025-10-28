//
// fluid.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Fluid rendering functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "hydra/fluid.h"
#include "atlas/core/shader.h"
#include "atlas/window.h"
#include <iostream>
#include <random>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

void Fluid::initialize() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(0.0f, bounds.x);
    std::uniform_real_distribution<float> distY(0.0f, bounds.y);
    std::uniform_real_distribution<float> distZ(0.0f, bounds.z);

    particles.reserve(numberParticles);
    for (int i = 0; i < numberParticles; ++i) {
        float x = distX(gen);
        float y = distY(gen);
        float z = 0;
        particles.push_back({glm::vec3(x, y, z)});
    }

    glEnable(GL_PROGRAM_POINT_SIZE);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(FluidParticle),
                 particles.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FluidParticle),
                          (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    fluidShader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Fluid,
                                                    AtlasFragmentShader::Fluid);

    initializeContainer();
}

void Fluid::resendParticles() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    particles.size() * sizeof(FluidParticle), particles.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Fluid::update(Window &window) {}

void Fluid::initializeContainer() {
    float vertices[] = {0.0f,
                        0.0f,
                        0.0f,
                        (float)bounds.x,
                        0.0f,
                        0.0f,
                        (float)bounds.x,
                        (float)bounds.y,
                        0.0f,
                        0.0f,
                        (float)bounds.y,
                        0.0f,
                        0.0f,
                        0.0f,
                        (float)bounds.z,
                        (float)bounds.x,
                        0.0f,
                        (float)bounds.z,
                        (float)bounds.x,
                        (float)bounds.y,
                        (float)bounds.z,
                        0.0f,
                        (float)bounds.y,
                        (float)bounds.z};

    unsigned int indices[] = {// Bottom face
                              0, 1, 1, 2, 2, 3, 3, 0,
                              // Top face
                              4, 5, 5, 6, 6, 7, 7, 4,
                              // Vertical edges
                              0, 4, 1, 5, 2, 6, 3, 7};

    glGenVertexArrays(1, &containerVAO);
    glGenBuffers(1, &containerVBO);
    glGenBuffers(1, &containerEBO);

    glBindVertexArray(containerVAO);

    glBindBuffer(GL_ARRAY_BUFFER, containerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, containerEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);

    glBindVertexArray(0);

    containerShader = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Basic, AtlasFragmentShader::Basic);
}

void Fluid::renderContainer() {
    glUseProgram(containerShader.programId);

    containerShader.setUniformMat4f(
        "projection", Window::mainWindow->calculateProjectionMatrix());
    containerShader.setUniformMat4f(
        "view", Window::mainWindow->camera->calculateViewMatrix());

    containerShader.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(containerVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Fluid::render(float dt) {
    // Render container first (as wireframe)
    renderContainer();

    // Then render particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(fluidShader.programId);
    fluidShader.setUniformMat4f(
        "projection", Window::mainWindow->calculateProjectionMatrix());
    fluidShader.setUniformMat4f(
        "view", Window::mainWindow->camera->calculateViewMatrix());

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}