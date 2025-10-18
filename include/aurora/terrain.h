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
#include "atlas/units.h"
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

    Position3d position;
    Rotation3d rotation;
    Scale3d scale = {1.0, 1.0, 1.0};

    void setPosition(const Position3d &newPosition) override {
        this->position = newPosition;
    };

    void setRotation(const Rotation3d &newRotation) override {
        this->rotation = newRotation;
    };

    void setScale(const Scale3d &newScale) override { this->scale = newScale; };

    void move(const Position3d &deltaPosition) override {
        this->position.x += deltaPosition.x;
        this->position.y += deltaPosition.y;
        this->position.z += deltaPosition.z;
    };

    void updateModelMatrix();

    void scaleBy(const Scale3d &deltaScale) {
        this->scale.x *= deltaScale.x;
        this->scale.y *= deltaScale.y;
        this->scale.z *= deltaScale.z;
    };

    void rotate(const Rotation3d &deltaRotation) override {
        this->rotation.pitch += deltaRotation.pitch;
        this->rotation.yaw += deltaRotation.yaw;
        this->rotation.roll += deltaRotation.roll;
    };

  private:
    BufferIndex vao = 0;
    BufferIndex vbo = 0;
    BufferIndex ebo = 0;
    ShaderProgram terrainShader;

    Texture terrainTexture;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    unsigned int patch_count;
    unsigned int rez;
};

#endif // AURORA_TERRAIN_H