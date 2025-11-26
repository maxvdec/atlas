//
// fluid.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Fluid implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "hydra/fluid.h"
#include "atlas/light.h"
#include "atlas/texture.h"
#include "atlas/window.h"

#include <cstddef>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

Fluid::Fluid() : GameObject() {
    renderLateForward = true;
    buildPlaneGeometry();
    updateModelMatrix();
}

Fluid::~Fluid() = default;

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

    vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                        sizeof(vertices), vertices.data());
    indexBuffer = opal::Buffer::create(opal::BufferUsage::IndexArray,
                                       sizeof(indices), indices.data());

    drawingState = opal::DrawingState::create(vertexBuffer, indexBuffer);
    drawingState->setBuffers(vertexBuffer, indexBuffer);

    const unsigned int stride = static_cast<unsigned int>(sizeof(FluidVertex));
    std::vector<opal::VertexAttributeBinding> bindings;
    bindings.reserve(5);

    auto makeBinding = [&](const char *name, unsigned int location,
                           unsigned int size, size_t offset) {
        opal::VertexAttribute attribute{std::string(name),
                                        opal::VertexAttributeType::Float,
                                        static_cast<unsigned int>(offset),
                                        location,
                                        false,
                                        size,
                                        stride,
                                        opal::VertexBindingInputRate::Vertex,
                                        0};
        bindings.push_back({attribute, vertexBuffer});
    };

    makeBinding("position", 0, 3, offsetof(FluidVertex, position));
    makeBinding("texCoord", 1, 2, offsetof(FluidVertex, texCoord));
    makeBinding("normal", 2, 3, offsetof(FluidVertex, normal));
    makeBinding("tangent", 3, 3, offsetof(FluidVertex, tangent));
    makeBinding("bitangent", 4, 3, offsetof(FluidVertex, bitangent));

    drawingState->configureAttributes(bindings);

    isInitialized = true;
}

void Fluid::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                   bool updatePipeline) {
    if (!isInitialized) {
        initialize();
    }

    (void)updatePipeline;
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "Fluid::render requires a valid command buffer");
    }
    if (captureDirty) {
        Window *window = Window::mainWindow;
        if (window) {
            ensureTargets(*window);
            if (reflectionTarget && refractionTarget) {
                window->captureFluidReflection(*this, commandBuffer);
                window->captureFluidRefraction(*this, commandBuffer);
                captureDirty = false;
            }
        }
    }

    glDisable(GL_CULL_FACE);

    static std::shared_ptr<opal::Pipeline> fluidPipeline = nullptr;
    if (fluidPipeline == nullptr) {
        fluidPipeline = opal::Pipeline::create();
    }
    fluidPipeline = fluidShader.requestPipeline(fluidPipeline);
    fluidPipeline->bind();

    fluidPipeline->setUniformMat4f("model", modelMatrix);
    fluidPipeline->setUniformMat4f("view", viewMatrix);
    fluidPipeline->setUniformMat4f("projection", projectionMatrix);
    fluidPipeline->setUniform4f("waterColor", color.r, color.g, color.b,
                                color.a);

    RenderTarget *target = Window::mainWindow->currentRenderTarget;
    if (target == nullptr) {
        return;
    }

    fluidPipeline->bindTexture2D("sceneTexture", target->texture.id, 0);
    fluidPipeline->bindTexture2D("sceneDepth", target->depthTexture.id, 1);

    if (reflectionTarget) {
        fluidPipeline->bindTexture2D("reflectionTexture",
                                     reflectionTarget->texture.id, 3);
    } else {
        fluidPipeline->bindTexture2D("reflectionTexture", target->texture.id,
                                     3);
    }

    if (refractionTarget) {
        fluidPipeline->bindTexture2D("refractionTexture",
                                     refractionTarget->texture.id, 4);
    } else {
        fluidPipeline->bindTexture2D("refractionTexture", target->texture.id,
                                     4);
    }

    fluidPipeline->bindTexture2D("movementTexture", movementTexture.id, 5);
    if (movementTexture.id == 0) {
        fluidPipeline->setUniform1i("hasMovementTexture", 0);
    } else {
        fluidPipeline->setUniform1i("hasMovementTexture", 1);
    }

    fluidPipeline->bindTexture2D("normalTexture", normalTexture.id, 6);
    fluidPipeline->setUniform1i("hasNormalTexture", normalTexture.id != 0);

    fluidPipeline->setUniform3f("cameraPos",
                                Window::mainWindow->getCamera()->position.x,
                                Window::mainWindow->getCamera()->position.y,
                                Window::mainWindow->getCamera()->position.z);

    fluidPipeline->setUniform3f("waterNormal", 0.0f, 1.0f, 0.0f);
    fluidPipeline->setUniform1f("time", dt);
    fluidPipeline->setUniform1f("refractionStrength", 0.5f);
    fluidPipeline->setUniform1f("reflectionStrength", 0.5f);
    fluidPipeline->setUniform1f("depthFade", 0.1f);

    DirectionalLight *primaryLight =
        Window::mainWindow->getCurrentScene()->directionalLights[0];

    fluidPipeline->setUniform3f("lightDirection", primaryLight->direction.x,
                                primaryLight->direction.y,
                                primaryLight->direction.z);

    fluidPipeline->setUniform3f("lightColor", primaryLight->color.r,
                                primaryLight->color.g, primaryLight->color.b);
    fluidPipeline->setUniform3f(
        "windForce", Window::mainWindow->getCurrentScene()->atmosphere.wind.x,
        Window::mainWindow->getCurrentScene()->atmosphere.wind.y,
        Window::mainWindow->getCurrentScene()->atmosphere.wind.z);

    fluidPipeline->setUniformMat4f("invProjection",
                                   glm::inverse(projectionMatrix));
    fluidPipeline->setUniformMat4f("invView", glm::inverse(viewMatrix));

    commandBuffer->bindDrawingState(drawingState);
    commandBuffer->drawIndexed(static_cast<unsigned int>(indices.size()), 1, 0,
                               0, 0);
    commandBuffer->unbindDrawingState();

    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_CULL_FACE);

    captureDirty = true;
}

void Fluid::update(Window &window) { (void)window; }

void Fluid::updateCapture(Window &window,
                          std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    ensureTargets(window);

    if (!reflectionTarget || !refractionTarget) {
        return;
    }

    window.captureFluidReflection(*this, commandBuffer);
    window.captureFluidRefraction(*this, commandBuffer);

    captureDirty = false;
}

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
    extentScale = {ext.width, 1.0f, ext.height};
    updateModelMatrix();
}

void Fluid::setWaterColor(const Color &newColor) { color = newColor; }

void Fluid::ensureTargets(Window &window) {
    auto refreshTarget = [&window](std::shared_ptr<RenderTarget> &target) {
        GLFWwindow *glfwWindow = static_cast<GLFWwindow *>(window.windowRef);
        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);
        float scale = window.getRenderScale();
        scale = std::clamp(scale, 0.1f, 1.0f);
        int desiredWidth =
            std::max(1, static_cast<int>(static_cast<float>(fbWidth) * scale));
        int desiredHeight =
            std::max(1, static_cast<int>(static_cast<float>(fbHeight) * scale));

        auto needsResize = [&](const std::shared_ptr<RenderTarget> &ptr) {
            if (!ptr) {
                return true;
            }
            return ptr->texture.creationData.width != desiredWidth ||
                   ptr->texture.creationData.height != desiredHeight;
        };

        if (needsResize(target)) {
            target =
                std::make_unique<RenderTarget>(window, RenderTargetType::Scene);

            GLint previousFBO;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

            glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        }
    };

    refreshTarget(reflectionTarget);
    refreshTarget(refractionTarget);
}

glm::vec3 Fluid::calculatePlanePoint() const {
    glm::vec4 worldPos = modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return glm::vec3(worldPos);
}

glm::vec3 Fluid::calculatePlaneNormal() const {
    glm::mat3 normalMatrix =
        glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    glm::vec3 normal = normalMatrix * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::length(normal) < 1e-5f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return glm::normalize(normal);
}

glm::vec4 Fluid::calculateClipPlane() const {
    glm::vec3 normal = calculatePlaneNormal();
    glm::vec3 point = calculatePlanePoint();
    float d = -glm::dot(normal, point);
    return glm::vec4(normal, d);
}

void Fluid::buildPlaneGeometry() {
    vertices = {FluidVertex{{-0.5f, 0.0f, -0.5f},
                            {0.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f}},
                FluidVertex{{0.5f, 0.0f, -0.5f},
                            {1.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f}},
                FluidVertex{{0.5f, 0.0f, 0.5f},
                            {1.0f, 1.0f},
                            {0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f}},
                FluidVertex{{-0.5f, 0.0f, 0.5f},
                            {0.0f, 1.0f},
                            {0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f}}};

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