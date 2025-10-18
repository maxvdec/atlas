//
// terrain.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Main terrain defintion and functions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef AURORA_TERRAIN_H
#define AURORA_TERRAIN_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/workspace.h"
#include <vector>

typedef unsigned int BufferIndex;

class Terrain : public GameObject {
  public:
    Resource heightmap;

    void render(float dt) override;
    void initialize() override;

    bool usesDeferredRendering = true;
    bool canUseDeferredRendering() const override { return false; }

    void setViewMatrix(const glm::mat4 &view) override { this->view = view; };
    void setProjectionMatrix(const glm::mat4 &projection) override {
        this->projection = projection;
    };

    Terrain(Resource heightmapResource) : heightmap(heightmapResource) {};
    Terrain() = default;

    int smoothness = 1;
    int detail = 1;

  private:
    BufferIndex vao = 0;
    BufferIndex vbo = 0;
    BufferIndex ebo = 0;
    ShaderProgram terrainShader;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int strip_count;
    int vertices_per_strip;
};

#endif // AURORA_TERRAIN_H