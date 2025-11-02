//
// fluid.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Water and fluid simulation
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef HYDRA_FLUID_H
#define HYDRA_FLUID_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <array>
#include <glm/glm.hpp>
#include <memory>

class Window;

struct Fluid : GameObject {
    float waveVelocity = 0.0f;

    Fluid();
    void create(Size2d extent, Color color);
    ~Fluid() override;

    void initialize() override;
    void update(Window &window) override;
    void render(float dt) override;

    /**
     * @brief Triggers the generation of reflection and refraction render
     * targets for the current frame.
     */
    void updateCapture(Window &window);

    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    void move(const Position3d &delta) override;
    void setPosition(const Position3d &pos) override;
    void setRotation(const Rotation3d &rot) override;
    void rotate(const Rotation3d &delta) override;
    void setScale(const Scale3d &scale) override;

    void setExtent(const Size2d &ext);
    void setWaveVelocity(float velocity) { waveVelocity = velocity; }
    void setWaterColor(const Color &color);

    Position3d getPosition() const override { return position; }
    Size3d getScale() const override { return scale; }

    bool canUseDeferredRendering() const override { return false; }

  private:
    struct FluidVertex {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;
    };

    void buildPlaneGeometry();
    void updateModelMatrix();
    glm::vec3 getFinalScale() const;

    Size2d extent{1.0, 1.0};
    Position3d position{0.0, 0.0, 0.0};
    Rotation3d rotation{0.0, 0.0, 0.0};
    Scale3d extentScale{1.0, 1.0, 1.0};
    Scale3d scale{1.0, 1.0, 1.0};
    Color color{1.0, 0.0, 0.0, 1.0};
    ShaderProgram fluidShader;

    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;

    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};

    std::array<FluidVertex, 4> vertices{};
    std::array<unsigned int, 6> indices{0, 1, 2, 0, 2, 3};
    bool isInitialized = false;

    std::shared_ptr<RenderTarget> reflectionTarget;
    std::shared_ptr<RenderTarget> refractionTarget;
    bool captureDirty = true;

    void ensureTargets(Window &window);
    glm::vec4 calculateClipPlane() const;
    glm::vec3 calculatePlaneNormal() const;
    glm::vec3 calculatePlanePoint() const;

    friend class Window;
};

#endif // HYDRA_FLUID_H