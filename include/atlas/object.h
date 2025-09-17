/*
 object.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Object properties and definitions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_OBJECT_H
#define ATLAS_OBJECT_H

#include "atlas/core/shader.h"
#include <atlas/units.h>
#include <vector>

struct CoreVertex {
    Position3d position;
};

typedef unsigned int BufferIndex;

class Renderable {
  public:
    virtual void render() = 0;
    virtual ~Renderable() = default;
};

class CoreObject : public Renderable {
  public:
    std::vector<CoreVertex> vertices;
    ShaderProgram shaderProgram;

    CoreObject();

    void attachVertices(const std::vector<CoreVertex> &newVertices);
    void attachProgram(const ShaderProgram &program);

  private:
    BufferIndex vbo;
    BufferIndex vao;

  public:
    void render() override;
};

#endif // ATLAS_OBJECT_H
