//
// fluid.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Defines fluid simulation structures
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef HYDRA_FLUID_H
#define HYDRA_FLUID_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "atlas/window.h"

struct FluidParticleGPU {
    glm::vec3 position;
    glm::vec3 velocity;
    float density;
    float mass;
};

struct FluidParticle {
    Position3d position;
    Magnitude3d velocity;
    float density;
    float mass;
};

class Fluid : public GameObject {
  public:
    void initialize() override;
    void update(Window &window) override;
    void render(float dt) override;

    Size3d bounds = Size3d(10.0f, 10.0f, 0.0f);
    int numberParticles = 1000;

    float gravity = 9.81f;

  private:
    Id vao = 0;
    Id vbo = 0;

    Id transformedVAO = 0;
    Id transformedVBO = 0;
    Id tfo = 0;

    Id containerVAO = 0;
    Id containerVBO = 0;
    Id containerEBO = 0;

    float particleSize = 0.1f;

    void resendParticles();
    void initializeContainer();
    void renderContainer();

    ShaderProgram fluidShader;
    ShaderProgram containerShader;
    ShaderProgram transformShader;
    std::vector<FluidParticle> particles;
};

#endif // HYDRA_FLUID_H