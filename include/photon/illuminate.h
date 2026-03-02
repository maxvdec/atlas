/*
 illuminate.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Global illumination and light management for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#ifndef PHOTON_ILLUMINATE_H
#define PHOTON_ILLUMINATE_H

#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "opal/opal.h"
#include <memory>

class GlobalIllumination {
  public:
    void render(std::shared_ptr<opal::CommandBuffer> commandBuffer);
    void init();

    std::shared_ptr<Texture> irradianceMap;
    std::shared_ptr<ShaderProgram> giShader;
    std::shared_ptr<opal::Pipeline> giPipeline;
};

#endif // PHOTON_ILLUMINATE_H
