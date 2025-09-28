//
// particle.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Particle system implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <cwchar>
#include <glad/glad.h>
#include <random>

ParticleEmitter::ParticleEmitter(unsigned int maxParticles)
    : maxParticles(maxParticles), vao(0), vbo(0), program({}), texture({}) {
    particles.resize(maxParticles);
    this->useTexture = false;
}

void ParticleEmitter::initialize() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle),
                 particles.data(), GL_DYNAMIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(Particle),
                          (void *)offsetof(Particle, position));
    glEnableVertexAttribArray(0);

    // Color
    glVertexAttribPointer(1, 4, GL_DOUBLE, GL_FALSE, sizeof(Particle),
                          (void *)offsetof(Particle, color));
    glEnableVertexAttribArray(1);

    // Size
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle),
                          (void *)offsetof(Particle, size));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    program = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Particle,
                                                AtlasFragmentShader::Particle);

    for (auto &p : particles) {
        respawnParticle(p, Position3d(0.0));
    }
}

void ParticleEmitter::respawnParticle(Particle &p,
                                      const Position3d &emitterPos) {
    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

    p.position = emitterPos;
    p.velocity =
        Magnitude3d{(rand01(rng) - 0.5f) * 2.0f, (rand01(rng) - 0.5f) * 2.0f,
                    (rand01(rng) - 0.5f) * 2.0f};
    p.color = color;
    p.life = 1.0f;
    p.size = minSize + (maxSize - minSize) * rand01(rng);
}

void ParticleEmitter::update(Window &window) {
    if (doesEmitOnce && emissions > 0) {
        return;
    }
    float dt = window.getDeltaTime();
    for (auto &p : particles) {
        p.color.a -= dt * 0.5f;
        p.life -= dt;
        if (p.life <= 0.0f) {
            respawnParticle(p, Position3d(0.0));
        } else {
            p.position = p.position + p.velocity * dt;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle),
                    particles.data());
    emissions++;
}

void ParticleEmitter::render(float dt) {
    glEnable(GL_PROGRAM_POINT_SIZE);

    glUseProgram(this->program.programId);
    program.setUniformMat4f("viewProj", this->view * this->projection);

    if (useTexture) {
        program.setUniform1i("useTexture", 1);
    } else {
        program.setUniform1i("useTexture", 0);
    }

    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        program.setUniform1i("particleText", 0);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);
}

void ParticleEmitter::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
}

void ParticleEmitter::setViewMatrix(const glm::mat4 &view) {
    this->view = view;
}

void ParticleEmitter::attachTexture(const Texture &tex) {
    this->texture = tex;
    this->useTexture = true;
}

void ParticleEmitter::setColor(const Color &color) { this->color = color; }

void ParticleEmitter::setPosition(const Position3d &newPosition) {
    this->position = newPosition;
}

void ParticleEmitter::move(const Position3d &deltaPosition) {
    this->position = this->position + deltaPosition;
}