//
// particle.cpp
// Billboard-based particle system implementation
// Created by Max Van den Eynde in 2025
//

#include "atlas/camera.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <cwchar>
#include <glad/glad.h>
#include <iostream>
#include <random>

struct QuadVertex {
    float x, y, z;
    float u, v;
};

struct ParticleInstanceData {
    float posX, posY, posZ;
    float colorR, colorG, colorB, colorA;
    float size;
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
    static const QuadVertex quadVertices[] = {{-0.5f, -0.5f, 0.0f, 0.0f, 0.0f},
                                              {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
                                              {0.5f, 0.5f, 0.0f, 1.0f, 1.0f},
                                              {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

    static const unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    unsigned int quadVBO, instanceVBO, EBO;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &EBO);

    vbo = instanceVBO;

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(ParticleInstanceData),
                 nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData), (void *)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                          sizeof(ParticleInstanceData),
                          (void *)(7 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

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
    } else if (emissionType == ParticleEmissionType::Ambient) {
        p.velocity.y += settings.gravity * 0.1f * deltaTime;

        float time = static_cast<float>(glfwGetTime());
        p.velocity.x +=
            sin(time * 1.0f + p.position.x * 0.1f) * 0.02f * deltaTime;
        p.velocity.z +=
            cos(time * 0.8f + p.position.z * 0.1f) * 0.02f * deltaTime;
    }

    p.position = p.position + p.velocity * deltaTime;

    if (emissionType == ParticleEmissionType::Ambient) {
        if (p.position.y < position.y - 15.0f ||
            abs(p.position.x - position.x) > 25.0f ||
            abs(p.position.z - position.z) > 25.0f) {
            p.position = generateSpawnPosition();
            p.velocity = generateRandomVelocity();
            p.life = p.maxLife;
        }
    }

    float lifeRatio = p.life / p.maxLife;
    p.color.a = lifeRatio;
}

Position3d ParticleEmitter::generateSpawnPosition() {
    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
    static std::uniform_real_distribution<float> randAngle(0.0f,
                                                           2.0f * 3.14159f);

    if (emissionType == ParticleEmissionType::Ambient) {
        Position3d spawnPos = position;

        spawnPos.x += (rand01(rng) - 0.5f) * 20.0f;
        spawnPos.z += (rand01(rng) - 0.5f) * 20.0f;
        spawnPos.y += rand01(rng) * 5.0f + 5.0f;

        return spawnPos;
    }

    if (spawnRadius <= 0.0f) {
        return position;
    }

    float angle = randAngle(rng);
    float radius = rand01(rng) * spawnRadius;

    Position3d spawnPos = position;
    spawnPos.x += cos(angle) * radius;
    spawnPos.z += sin(angle) * radius;

    return spawnPos;
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
        vel.x = (rand01(rng) - 0.5f) * 0.5f;
        vel.y = -0.5f - rand01(rng) * 1.0f;
        vel.z = (rand01(rng) - 0.5f) * 0.5f;
    }

    float speed = 1.0f + (rand01(rng) - 0.5f) * settings.speedVariation;
    vel.x *= speed;
    vel.y *= speed;
    vel.z *= speed;

    return vel;
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

    float baseLifetime =
        settings.minLifetime +
        (settings.maxLifetime - settings.minLifetime) * rand01(rng);

    float heightMultiplier = 1.0f;
    if (firstCameraPosition.has_value()) {
        float currentCameraY = model[3][1];
        float initialCameraY = firstCameraPosition->y;

        float heightDifference = currentCameraY - initialCameraY;

        heightMultiplier =
            std::clamp(1.0f + heightDifference * 0.1f, 1.0f, 3.0f);
    }

    p.life = baseLifetime * heightMultiplier;
    p.maxLife = p.life;
    p.size =
        settings.minSize + (settings.maxSize - settings.minSize) * rand01(rng);
}

void ParticleEmitter::update(Window &window) {
    float dt = window.getDeltaTime();
    Camera *cam = window.getCamera();
    this->model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(cam->position.x, cam->position.y, cam->position.z));
    if (!firstCameraPosition.has_value()) {
        firstCameraPosition = cam->position;
    }

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

    for (auto &p : particles) {
        updateParticle(p, dt);
    }

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
    glDepthMask(GL_FALSE);

    glUseProgram(this->program.programId);

    program.setUniformMat4f("view", this->view);
    program.setUniformMat4f("projection", this->projection);
    program.setUniformMat4f("model", this->model);
    program.setUniform1i("useTexture", useTexture ? 1 : 0);

    program.setUniform1i(
        "isAmbient", (emissionType == ParticleEmissionType::Ambient) ? 1 : 0);

    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        program.setUniform1i("particleTexture", 0);
    }

    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                            activeParticleCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
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
    ParticleSettings settings;
    settings.gravity = -1.0f;
    settings.minSize = 0.04f;
    settings.maxSize = 0.07f;
    settings.minLifetime = 5.0f;
    settings.maxLifetime = 10.0f;
    setParticleSettings(settings);
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