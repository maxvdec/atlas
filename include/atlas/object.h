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

#include <glm/glm.hpp>

typedef std::array<double, 2> TextureCoordinate;

struct CoreVertex {
    Position3d position;
    Color color = {1.0, 1.0, 1.0, 1.0};
    TextureCoordinate textureCoordinate = {0.0, 0.0};
    Normal3d normal = {0.0, 0.0, 0.0};

    static std::vector<LayoutDescriptor> getLayoutDescriptors();
};

typedef unsigned int BufferIndex;
typedef unsigned int Index;

class Renderable {
  public:
    virtual void render() = 0;
    virtual void initialize() = 0;
    virtual void setViewMatrix(const glm::mat4 &view) = 0;
    virtual void setProjectionMatrix(const glm::mat4 &projection) = 0;
    virtual ~Renderable() = default;
};

class CoreObject : public Renderable {
  public:
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
    ShaderProgram shaderProgram;
    std::vector<Texture> textures;

    CoreObject();

    void attachVertices(const std::vector<CoreVertex> &newVertices);
    void attachIndices(const std::vector<Index> &newIndices);
    void attachProgram(const ShaderProgram &program);
    void attachTexture(const Texture &texture);
    void initialize() override;

    void renderColorWithTexture();
    void renderOnlyColor();
    void renderOnlyTexture();

    Position3d position = {0.0, 0.0, 0.0};
    Rotation3d rotation = {0.0, 0.0, 0.0};
    Scale3d scale = {1.0, 1.0, 1.0};

    void setPosition(const Position3d &newPosition);
    void move(const Position3d &deltaPosition);
    void setRotation(const Rotation3d &newRotation);
    void rotate(const Rotation3d &deltaRotation);
    void setScale(const Scale3d &newScale);

    void updateModelMatrix();

    CoreObject clone() const;

  private:
    BufferIndex vbo;
    BufferIndex vao;
    BufferIndex ebo;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    bool useColor = true;
    bool useTexture = false;

  public:
    void render() override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;
};

CoreObject createBox(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
CoreObject createPlane(Size2d size, Color color = {1.0, 1.0, 1.0, 1.0});

#endif // ATLAS_OBJECT_H
