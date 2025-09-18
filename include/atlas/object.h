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

    static std::vector<LayoutDescriptor> getLayoutDescriptors();
};

typedef unsigned int BufferIndex;
typedef unsigned int Index;

class Renderable {
  public:
    virtual void render() = 0;
    virtual void initialize() = 0;
    virtual ~Renderable() = default;
};

class CoreObject : public Renderable {
  public:
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
    ShaderProgram shaderProgram;

    CoreObject();

    void attachVertices(const std::vector<CoreVertex> &newVertices);
    void attachIndices(const std::vector<Index> &newIndices);
    void attachProgram(const ShaderProgram &program);
    void initialize() override;

  private:
    BufferIndex vbo;
    BufferIndex vao;
    BufferIndex ebo;

  public:
    void render() override;
};

#endif // ATLAS_OBJECT_H
