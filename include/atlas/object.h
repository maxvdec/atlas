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
#include "atlas/texture.h"
#include <array>
#include <atlas/units.h>
#include <optional>
#include <vector>

typedef std::array<double, 2> TextureCoordinate;

struct CoreVertex {
    Position3d position;
    Color color = {1.0, 1.0, 1.0, 1.0};
    TextureCoordinate textureCoordinate = {0.0, 0.0};

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
    std::optional<Texture> texture;

    CoreObject();

    void attachVertices(const std::vector<CoreVertex> &newVertices);
    void attachIndices(const std::vector<Index> &newIndices);
    void attachProgram(const ShaderProgram &program);
    void attachTexture(const Texture &texture);
    void initialize() override;

    void renderColorWithTexture();

  private:
    BufferIndex vbo;
    BufferIndex vao;
    BufferIndex ebo;

    bool onlyTexture = true;

  public:
    void render() override;
};

#endif // ATLAS_OBJECT_H
