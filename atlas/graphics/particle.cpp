//
// particle.cpp
// Billboard-based particle system implementation
// Created by Max Van den Eynde in 2025
//

#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <cwchar>
#include <glad/glad.h>
#include <iostream>
#include <random>

// Quad vertex structure for billboard rendering
struct QuadVertex {
    float x, y, z; // Position
    float u, v;    // Texture coordinates
};

// Instance data structure (matches shader layout)
struct ParticleInstanceData {
    float posX, posY, posZ;               // Position (location = 2)
    float colorR, colorG, colorB, colorA; // Color (location = 3)
    float size;                           // Size (location = 4)
};

ParticleEmitter::ParticleEmitter(unsigned int maxParticles)
    : maxParticles(maxParticles), vao(0), vbo(0), program({}), texture({}) {
    particles.resize(maxParticles);
    this->useTexture = false;
    this->emissionType = ParticleEmissionType::Fountain;
    this->direction = {0.0, 1.0, 0.0};
    this->spawnRadius = 0.1f;
    this->spawnRate = 10.0f;
    this->timeSinceLastEmission = 0.0f;
    this->isEmitting = true;
    this->doesEmitOnce = false;
    this->hasEmittedOnce = false;
    this->burstCount = 0;

    for (auto &p : particles) {
        p.active = false;
    }
}

void ParticleEmitter::initialize() {
    // Create quad vertices (billboard quad from -0.5 to 0.5)
    static const QuadVertex quadVertices[] = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f}, // Bottom-left
        {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},  // Bottom-right
        {0.5f, 0.5f, 0.0f, 1.0f, 1.0f},   // Top-right
        {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}   // Top-left
    };

    static const unsigned int indices[] = {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
    };

    // Generate buffers
    unsigned int quadVBO, instanceVBO, EBO;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &EBO);

    // Store instance VBO for later updates
    vbo = instanceVBO;

    glBindVertexArray(vao);

    // Setup quad vertices (shared by all particles)
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    // Quad vertex position (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void *)0);
    glEnableVertexAttribArray(0);

    // Quad texture coordinates (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Setup element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    // Setup instance buffer (particle data)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(ParticleInstanceData),
                 nullptr, GL_DYNAMIC_DRAW);

    // Instance position (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData), (void *)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // One per instance

    // Instance color (location = 3)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1); // One per instance

    // Instance size (location = 4)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData),
                          (void *)(7 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1); // One per instance

    glBindVertexArray(0);

    // Create billboard shader program
    program = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Particle,
                                                AtlasFragmentShader::Particle);
}

void ParticleEmitter::spawnParticle() {
    int index = findInactiveParticle();
    if (index != -1) {
        activateParticle(index);
    }
}

void ParticleEmitter::updateParticle(Particle &p, float deltaTime) {
    if (!p.active)
        return;

    p.life -= deltaTime;
    if (p.life <= 0.0f) {
        p.active = false;
        return;
    }

    if (emissionType == ParticleEmissionType::Fountain) {
        p.velocity.y += settings.gravity * deltaTime;
    }

    p.position = p.position + p.velocity * deltaTime;

    float lifeRatio = p.life / p.maxLife;
    p.color.a = lifeRatio;
}

Magnitude3d ParticleEmitter::generateRandomVelocity() {
    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    static std::uniform_real_distribution<float> randAngle(0.0f,
                                                           2.0f * 3.14159f);

    Magnitude3d vel = direction;

    if (emissionType == ParticleEmissionType::Fountain) {
        float spreadX = (rand01(rng) - 0.5f) * settings.spread;
        float spreadZ = (rand01(rng) - 0.5f) * settings.spread;
        vel.x += spreadX;
        vel.z += spreadZ;
    } else if (emissionType == ParticleEmissionType::Ambient) {
        float angle = randAngle(rng);
        float elevation = (rand01(rng) - 0.5f) * 3.14159f;
        vel.x = cos(angle) * cos(elevation);
        vel.y = sin(elevation);
        vel.z = sin(angle) * cos(elevation);
    }

    float speed = 1.0f + (rand01(rng) - 0.5f) * settings.speedVariation;
    vel.x *= speed;
    vel.y *= speed;
    vel.z *= speed;

    return vel;
}

Position3d ParticleEmitter::generateSpawnPosition() {
    if (spawnRadius <= 0.0f) {
        return position;
    }

    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    static std::uniform_real_distribution<float> randAngle(0.0f,
                                                           2.0f * 3.14159f);

    float angle = randAngle(rng);
    float radius = rand01(rng) * spawnRadius;

    Position3d spawnPos = position;
    spawnPos.x += cos(angle) * radius;
    spawnPos.z += sin(angle) * radius;

    return spawnPos;
}

int ParticleEmitter::findInactiveParticle() {
    for (int i = 0; i < particles.size(); ++i) {
        if (!particles[i].active) {
            return i;
        }
    }
    return -1;
}

void ParticleEmitter::activateParticle(int index) {
    if (index < 0 || index >= particles.size())
        return;

    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

    Particle &p = particles[index];
    p.active = true;
    p.position = generateSpawnPosition();
    p.velocity = generateRandomVelocity();
    p.color = color;
    p.life = settings.minLifetime +
             (settings.maxLifetime - settings.minLifetime) * rand01(rng);
    p.maxLife = p.life;
    p.size =
        settings.minSize + (settings.maxSize - settings.minSize) * rand01(rng);
}

void ParticleEmitter::update(Window &window) {
    float dt = window.getDeltaTime();

    if (isEmitting && !hasEmittedOnce) {
        timeSinceLastEmission += dt;

        if (burstCount > 0) {
            for (int i = 0; i < burstCount; ++i) {
                spawnParticle();
            }
            burstCount = 0;
        }

        if (!doesEmitOnce) {
            float emissionInterval = 1.0f / spawnRate;
            while (timeSinceLastEmission >= emissionInterval) {
                spawnParticle();
                timeSinceLastEmission -= emissionInterval;
            }
        } else if (doesEmitOnce && !hasEmittedOnce) {
            for (int i = 0; i < maxParticles / 2; ++i) {
                spawnParticle();
            }
            hasEmittedOnce = true;
        }
    }

    // Update all particles
    for (auto &p : particles) {
        updateParticle(p, dt);
    }

    // Prepare instance data for GPU upload
    std::vector<ParticleInstanceData> instanceData;
    instanceData.reserve(maxParticles);

    for (const auto &p : particles) {
        if (p.active) {
            ParticleInstanceData data;
            data.posX = static_cast<float>(p.position.x);
            data.posY = static_cast<float>(p.position.y);
            data.posZ = static_cast<float>(p.position.z);
            data.colorR = static_cast<float>(p.color.r);
            data.colorG = static_cast<float>(p.color.g);
            data.colorB = static_cast<float>(p.color.b);
            data.colorA = static_cast<float>(p.color.a);
            data.size = p.size;
            instanceData.push_back(data);
        }
    }

    // Update GPU buffer with active particles
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (!instanceData.empty()) {
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        instanceData.size() * sizeof(ParticleInstanceData),
                        instanceData.data());
    }

    activeParticleCount = instanceData.size();
}

void ParticleEmitter::render(float dt) {
    if (activeParticleCount == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer

    glUseProgram(this->program.programId);

    // Set uniforms
    program.setUniformMat4f("view", this->view);
    program.setUniformMat4f("projection", this->projection);
    program.setUniform1i("useTexture", useTexture ? 1 : 0);

    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        program.setUniform1i("particleTexture", 0);
    }

    // Render billboards
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                            activeParticleCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE); // Re-enable depth writing
    glDisable(GL_BLEND);
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

void ParticleEmitter::setEmissionType(ParticleEmissionType type) {
    this->emissionType = type;
}

void ParticleEmitter::setDirection(const Magnitude3d &dir) {
    this->direction = dir;
}

void ParticleEmitter::setSpawnRadius(float radius) {
    this->spawnRadius = radius;
}

void ParticleEmitter::setSpawnRate(float particlesPerSecond) {
    this->spawnRate = particlesPerSecond;
}

void ParticleEmitter::setParticleSettings(const ParticleSettings &settings) {
    this->settings = settings;
}

void ParticleEmitter::emitOnce() {
    this->doesEmitOnce = true;
    this->hasEmittedOnce = false;
    this->isEmitting = true;
}

void ParticleEmitter::emitContinuously() {
    this->doesEmitOnce = false;
    this->hasEmittedOnce = false;
    this->isEmitting = true;
}

void ParticleEmitter::startEmission() { this->isEmitting = true; }

void ParticleEmitter::stopEmission() { this->isEmitting = false; }

void ParticleEmitter::emitBurst(int count) { this->burstCount = count; }