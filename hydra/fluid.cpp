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
#include "atlas/units.h"
#include "atlas/window.h"
#include <iostream>
#include <random>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

void Fluid::initialize() {
    particles.reserve(numberParticles);

    int particlesPerRow = std::ceil(std::sqrt(numberParticles));
    float spacingX = bounds.x / (particlesPerRow + 1);
    float spacingY = bounds.y / (particlesPerRow + 1);

    int count = 0;
    for (int i = 0; i < particlesPerRow && count < numberParticles; ++i) {
        for (int j = 0; j < particlesPerRow && count < numberParticles; ++j) {
            float x = (i + 1) * spacingX;
            float y = (j + 1) * spacingY;
            float z = 0.0f;

            particles.push_back({Position3d(x, y, z),
                                 Magnitude3d(0.0f, 0.0f, 0.0f), 0.0f, 1.0f});
            count++;
        }
    }

    particleSize = std::min(spacingX, spacingY) * 0.5f;

    std::vector<FluidParticleGPU> gpuParticles;
    gpuParticles.reserve(numberParticles);
    for (const auto &p : particles) {
        gpuParticles.push_back(
            {glm::vec3(p.position.x, p.position.y, p.position.z),
             glm::vec3(p.velocity.x, p.velocity.y, p.velocity.z), p.density,
             p.mass});
    }

    glEnable(GL_PROGRAM_POINT_SIZE);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 gpuParticles.size() * sizeof(FluidParticleGPU),
                 gpuParticles.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)0);

    // Velocity attribute (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, velocity));
    // Density attribute (1 float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, density));
    // Mass attribute (1 float)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, mass));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &transformedVAO);
    glGenBuffers(1, &transformedVBO);
    glBindVertexArray(transformedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transformedVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 gpuParticles.size() * sizeof(FluidParticleGPU),
                 gpuParticles.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)0);

    // Velocity attribute (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, velocity));
    // Density attribute (1 float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, density));
    // Mass attribute (1 float)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(FluidParticleGPU),
                          (void *)offsetof(FluidParticleGPU, mass));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTransformFeedbacks(1, &tfo);

    transformShader = ShaderProgram::transformFromDefaultShader(
        AtlasVertexShader::Fluid, AtlasFragmentShader::Empty,
        {"outPosition", "outVelocity", "outDensity", "outMass"});

    GeometryShader gShader =
        GeometryShader::fromDefaultShader(AtlasGeometryShader::RenderFluid);
    gShader.compile();
    fluidShader = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::RenderFluid, AtlasFragmentShader::RenderFluid,
        gShader);

    initializeContainer();
}

void Fluid::resendParticles() {
    std::vector<FluidParticleGPU> gpuParticles;
    gpuParticles.reserve(particles.size());
    for (const auto &p : particles) {
        gpuParticles.push_back(
            {glm::vec3(p.position.x, p.position.y, p.position.z),
             glm::vec3(p.velocity.x, p.velocity.y, p.velocity.z), p.density,
             p.mass});
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    gpuParticles.size() * sizeof(FluidParticleGPU),
                    gpuParticles.data());
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
    renderContainer();

    glUseProgram(transformShader.programId);
    transformShader.setUniform1f("dt", dt);
    transformShader.setUniform1f("gravity", gravity);
    transformShader.setUniform1f("particleSize", particleSize);
    transformShader.setUniform3f("bounds", (float)bounds.x, (float)bounds.y,
                                 (float)bounds.z);

    glBindVertexArray(vao);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, transformedVBO);

    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);

    glBindVertexArray(0);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    std::swap(vao, transformedVAO);
    std::swap(vbo, transformedVBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(fluidShader.programId);
    fluidShader.setUniformMat4f(
        "projection", Window::mainWindow->calculateProjectionMatrix());
    fluidShader.setUniformMat4f(
        "view", Window::mainWindow->camera->calculateViewMatrix());
    fluidShader.setUniform1f("particleSize", particleSize);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}