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
#include <cstddef>
#include <glad/glad.h>
#include <random>
#include "atlas/particle.h"

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
    : maxParticles(maxParticles), vao(nullptr), quadBuffer(nullptr),
      instanceBuffer(nullptr), indexBuffer(nullptr), program({}), texture({}) {
    particles.reserve(maxParticles);
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

    quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                      sizeof(quadVertices), quadVertices);
    indexBuffer = opal::Buffer::create(opal::BufferUsage::IndexArray,
                                       sizeof(indices), indices);
    instanceBuffer =
        opal::Buffer::create(opal::BufferUsage::GeneralPurpose,
                             maxParticles * sizeof(ParticleInstanceData),
                             nullptr, opal::MemoryUsageType::CPUToGPU);

    vao = opal::DrawingState::create(quadBuffer, indexBuffer);
    vao->setBuffers(quadBuffer, indexBuffer);

    opal::VertexAttribute positionAttr{"particlePosition",
                                       opal::VertexAttributeType::Float,
                                       0,
                                       0,
                                       false,
                                       3,
                                       static_cast<uint>(sizeof(QuadVertex)),
                                       opal::VertexBindingInputRate::Vertex,
                                       0};
    opal::VertexAttribute uvAttr{"particleUV",
                                 opal::VertexAttributeType::Float,
                                 static_cast<uint>(3 * sizeof(float)),
                                 1,
                                 false,
                                 2,
                                 static_cast<uint>(sizeof(QuadVertex)),
                                 opal::VertexBindingInputRate::Vertex,
                                 0};
    opal::VertexAttribute instancePos{
        "instancePosition",
        opal::VertexAttributeType::Float,
        0,
        2,
        false,
        3,
        static_cast<uint>(sizeof(ParticleInstanceData)),
        opal::VertexBindingInputRate::Instance,
        1};
    opal::VertexAttribute instanceColor{
        "instanceColor",
        opal::VertexAttributeType::Float,
        static_cast<uint>(3 * sizeof(float)),
        3,
        false,
        4,
        static_cast<uint>(sizeof(ParticleInstanceData)),
        opal::VertexBindingInputRate::Instance,
        1};
    opal::VertexAttribute instanceSize{
        "instanceSize",
        opal::VertexAttributeType::Float,
        static_cast<uint>(7 * sizeof(float)),
        4,
        false,
        1,
        static_cast<uint>(sizeof(ParticleInstanceData)),
        opal::VertexBindingInputRate::Instance,
        1};

    std::vector<opal::VertexAttributeBinding> bindings = {
        {positionAttr, quadBuffer},
        {uvAttr, quadBuffer},
        {instancePos, instanceBuffer},
        {instanceColor, instanceBuffer},
        {instanceSize, instanceBuffer}};
    vao->configureAttributes(bindings);

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

    Magnitude3d windDirection = Magnitude3d{0.0f, 0.0f, 0.0f};
    if (Window::mainWindow->getCurrentScene()->atmosphere.isEnabled()) {
        windDirection = Window::mainWindow->getCurrentScene()->atmosphere.wind;
    }

    p.velocity.x += windDirection.x * deltaTime;
    p.velocity.y += windDirection.y * deltaTime;
    p.velocity.z += windDirection.z * deltaTime;

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
    for (size_t i = 0; i < particles.size(); ++i) {
        if (!particles[i].active) {
            return i;
        }
    }
    return -1;
}

void ParticleEmitter::activateParticle(int index) {
    if (index < 0 || index >= static_cast<int>(particles.size()))
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
            for (size_t i = 0; i < maxParticles / 2; ++i) {
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

    if (instanceBuffer != nullptr && !instanceData.empty()) {
        instanceBuffer->bind();
        instanceBuffer->updateData(
            0, instanceData.size() * sizeof(ParticleInstanceData),
            instanceData.data());
        instanceBuffer->unbind();
    }

    activeParticleCount = instanceData.size();
}

void ParticleEmitter::render(float dt,
                             std::shared_ptr<opal::CommandBuffer> commandBuffer,
                             bool updatePipeline) {
    (void)updatePipeline;
    for (auto &component : components) {
        component->update(dt);
    }
    if (activeParticleCount == 0)
        return;
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "ParticleEmitter::render requires a valid command buffer");
    }

    // Get or create pipeline
    static std::shared_ptr<opal::Pipeline> particlePipeline = nullptr;
    if (particlePipeline == nullptr) {
        particlePipeline = opal::Pipeline::create();
    }
    particlePipeline = this->program.requestPipeline(particlePipeline);
    particlePipeline->enableBlending(true);
    particlePipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                   opal::BlendFunc::OneMinusSrcAlpha);
    particlePipeline->enableDepthWrite(false);
    particlePipeline->bind();

    particlePipeline->setUniformMat4f("view", this->view);
    particlePipeline->setUniformMat4f("projection", this->projection);
    particlePipeline->setUniformMat4f("model", this->model);
    particlePipeline->setUniform1i("useTexture", useTexture ? 1 : 0);

    particlePipeline->setUniform1i(
        "isAmbient", (emissionType == ParticleEmissionType::Ambient) ? 1 : 0);

    if (useTexture) {
        particlePipeline->bindTexture2D("particleTexture", texture.id, 0);
    }

    commandBuffer->bindDrawingState(vao);
    commandBuffer->drawIndexed(6, activeParticleCount, 0, 0, 0);
    commandBuffer->unbindDrawingState();

    particlePipeline->enableDepthWrite(true);
    particlePipeline->enableBlending(false);
    particlePipeline->bind();
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