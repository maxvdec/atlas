/*
 renderable.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Renderable definition and concept
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_RENDERABLE_H
#define ATLAS_RENDERABLE_H

#include "atlas/core/shader.h"
#include "atlas/units.h"
#include <glm/glm.hpp>
#include <optional>
#include <vector>

struct CoreVertex;
class Window;

/**
 * @brief An abstract base class representing any object that can be rendered in
 * a Window. Contains virtual methods for rendering, initialization, updating,
 * and setting view/projection matrices.
 *
 */
class Renderable {
  public:
    /**
     * @brief Function to render the object. Must be implemented by derived
     * classes.
     *
     * @param dt Delta time since the last frame, useful for animations.
     */
    virtual void render(float dt) = 0;
    /**
     * @brief Function to initialize the object. Can be overridden by derived.
     *
     */
    virtual void initialize() {};
    /**
     * @brief Function to update the object each frame.
     * \warning It runs before the rendering phase and it should only contain
     * logic updates.
     *
     * @param window The window where the object is being rendered.
     */
    virtual void update(Window &window) {};
    /**
     * @brief Function to set the view matrix for the object. Called from \ref
     * Window for internal purposes.
     *
     * @param view The view matrix to set.
     */
    virtual void setViewMatrix(const glm::mat4 &view) {};
    /**
     * @brief Function to set the projection matrix for the object. Called from
     * \ref Window for internal purposes.
     *
     * @param projection The projection matrix to set.
     */
    virtual void setProjectionMatrix(const glm::mat4 &projection) {};
    /**
     * @brief Function to get the current shader program used by the object.
     *
     * @return An optional containing the shader program if set, or \c
     * std::nullopt if not.
     */
    virtual std::optional<ShaderProgram> getShaderProgram() {
        return std::nullopt;
    };
    /**
     * @brief Function to set the shader program for the object. Can be used to
     * force an object to use a specific shader.
     *
     * @param shader The shader program to set.
     */
    virtual void setShader(const ShaderProgram &shader) {};
    /**
     * @brief Function to get the position of the object in 3D space.
     *
     * @return The position of the object as a Position3d struct.
     */
    virtual Position3d getPosition() const { return {0, 0, 0}; };
    /**
     * @brief Function to get the vertices of the object in 3D space.
     *
     * @return The vertices of the object as a vector of CoreVertex structs.
     */
    virtual std::vector<CoreVertex> getVertices() const { return {}; };
    /**
     * @brief Function to get the scale of the object in 3D space.
     *
     * @return The scale of the object as a Size3d struct.
     */
    virtual Size3d getScale() const { return {1, 1, 1}; };
    /**
     * @brief Function to determine if the object can cast shadows. Can be
     * overridden by derived classes.
     *
     * @return true if the object can cast shadows, false otherwise.
     */
    virtual bool canCastShadows() const { return false; };
    virtual ~Renderable() = default;

    bool renderDepthOfView = false;
};

#endif // ATLAS_RENDERABLE_H
