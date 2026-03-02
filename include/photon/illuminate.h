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
#include "atlas/units.h"
#include "opal/opal.h"
#include <memory>

namespace photon {

struct ProbeSpace {
    Position3d originWorldSpace;
    Position3d spacing;
    Vector3 probeCount;
    Color debugColor;

    int textureBorderSize = 1;
    int probeResolution = 8;
    int probesPerRow = 16;

    int totalProbes() const {
        return static_cast<int>(probeCount.x * probeCount.y * probeCount.z);
    }

    int tileResolution() const {
        return probeResolution + (2 * textureBorderSize);
    }

    int atlasRows() const {
        return (totalProbes() + probesPerRow - 1) / probesPerRow;
    }

    int atlasWidth() const { return probesPerRow * tileResolution(); }

    int atlasHeight() const { return atlasRows() * tileResolution(); }
};

class GlobalIllumination {
  public:
    void
    render(const std::shared_ptr<opal::CommandBuffer> &commandBuffer) const;
    void updateProbeLayout();
    void init();

    std::shared_ptr<Texture> irradianceMap;
    std::shared_ptr<ShaderProgram> giShader;
    std::shared_ptr<opal::Pipeline> giPipeline;

    std::shared_ptr<ProbeSpace> probeSpace;

    float probeSpacing = 0.5f;
};

} // namespace photon

#endif // PHOTON_ILLUMINATE_H
