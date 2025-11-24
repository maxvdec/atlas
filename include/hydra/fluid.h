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

/**
 * @brief Dynamic water surface that captures reflections/refractions and
 * renders Gerstner-style waves.
 */
struct Fluid : GameObject {
    /**
     * @brief Speed multiplier for texture-driven wave animation.
     */
    float waveVelocity = 0.0f;

    /**
     * @brief Constructs an uninitialized fluid surface.
     */
    Fluid();
    /**
     * @brief Builds the fluid mesh with a given extent and base color.
     */
    void create(Size2d extent, Color color);
    /**
     * @brief Releases GPU buffers and render targets.
     */
    ~Fluid() override;

    /**
     * @brief Allocates GPU buffers and prepares render targets.
     */
    void initialize() override;
    /**
     * @brief Advances water simulation state and schedules capture updates.
     */
    void update(Window &window) override;
    /**
     * @brief Draws the water surface and applies screen-space effects.
     */
    void render(float dt, bool updatePipeline = false) override;

    /**
     * @brief Triggers the generation of reflection and refraction render
     * targets for the current frame.
     */
    void updateCapture(Window &window);

    /**
     * @brief Stores the camera view matrix for shader consumption.
     */
    void setViewMatrix(const glm::mat4 &view) override;
    /**
     * @brief Saves the projection matrix used to render the water surface.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /**
     * @brief Translates the water plane by the provided delta.
     */
    void move(const Position3d &delta) override;
    /**
     * @brief Sets the absolute world-space position.
     */
    void setPosition(const Position3d &pos) override;
    /**
     * @brief Applies an absolute Euler rotation to the surface.
     */
    void setRotation(const Rotation3d &rot) override;
    /**
     * @brief Adds an incremental rotation on top of the current orientation.
     */
    void rotate(const Rotation3d &delta) override;
    /**
     * @brief Sets a custom scale factor applied after the physical extent.
     */
    void setScale(const Scale3d &scale) override;

    /**
     * @brief Applies a new width/height for the water plane.
     */
    void setExtent(const Size2d &ext);
    /**
     * @brief Adjusts the scroll speed of the normal/movement textures.
     */
    void setWaveVelocity(float velocity) { waveVelocity = velocity; }
    /**
     * @brief Changes the albedo tint used when shading the surface.
     */
    void setWaterColor(const Color &color);

    /**
     * @brief Returns the current world position of the fluid surface.
     */
    Position3d getPosition() const override { return position; }
    /**
     * @brief Returns the scale applied after combining extent and user scale.
     */
    Size3d getScale() const override { return scale; }

    /**
     * @brief Fluids are always rendered in forward passes, so deferred is
     * disabled.
     */
    bool canUseDeferredRendering() override { return false; }

    /**
     * @brief Normal map used for lighting perturbations.
     */
    Texture normalTexture;
    /**
     * @brief Flow map used to animate surface movement.
     */
    Texture movementTexture;

  private:
    struct FluidVertex {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
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