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

class Renderable {
  public:
    virtual void render(float dt) = 0;
    virtual void initialize() {};
    virtual void update(Window &window) {};
    virtual void setViewMatrix(const glm::mat4 &view) {};
    virtual void setProjectionMatrix(const glm::mat4 &projection) {};
    virtual std::optional<ShaderProgram> getShaderProgram() {
        return std::nullopt;
    };
    virtual void setShader(const ShaderProgram &shader) {};
    virtual Position3d getPosition() const { return {0, 0, 0}; };
    virtual std::vector<CoreVertex> getVertices() const { return {}; };
    virtual Size3d getScale() const { return {1, 1, 1}; };
    virtual bool canCastShadows() const { return false; };
    virtual ~Renderable() = default;
};

#endif // ATLAS_RENDERABLE_H
