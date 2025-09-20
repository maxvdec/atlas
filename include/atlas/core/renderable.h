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

#include <glm/glm.hpp>

class Renderable {
  public:
    virtual void render() = 0;
    virtual void initialize() {};
    virtual void setViewMatrix(const glm::mat4 &view) {};
    virtual void setProjectionMatrix(const glm::mat4 &projection) {};
    virtual ~Renderable() = default;
};

#endif // ATLAS_RENDERABLE_H
