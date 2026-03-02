/*
 gi.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: DDGI Implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include "photon/illuminate.h"
#include <algorithm>
#include <cmath>
#include <memory>

void photon::GlobalIllumination::init() {
    ComputeShader ddgiShader =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI);
    ddgiShader.compile();
    giShader = std::make_shared<ShaderProgram>();
    giShader->computeShader = ddgiShader;
    giShader->compile();

    irradianceMap = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    giPipeline = opal::Pipeline::create();
    giPipeline->setShaderProgram(giShader->shader);
    giPipeline->setComputeThreadgroupSize(8, 8, 1);
    giPipeline->build();

    probeSpace = std::make_shared<ProbeSpace>();
}

void photon::GlobalIllumination::updateProbeLayout() {
    if (probeSpace == nullptr || Window::mainWindow == nullptr) {
        return;
    }

    BoundingBox sceneBounds = Window::mainWindow->getSceneBoundingBox();

    float spacing = std::max(0.001f, probeSpacing);

    const bool boundsAreFinite =
        std::isfinite(sceneBounds.min.x) && std::isfinite(sceneBounds.min.y) &&
        std::isfinite(sceneBounds.min.z) && std::isfinite(sceneBounds.max.x) &&
        std::isfinite(sceneBounds.max.y) && std::isfinite(sceneBounds.max.z);

    Position3d minWs = boundsAreFinite
                           ? Position3d(sceneBounds.min.x - spacing,
                                        sceneBounds.min.y - spacing,
                                        sceneBounds.min.z - spacing)
                           : Position3d(-spacing, -spacing, -spacing);

    Position3d maxWs = boundsAreFinite ? Position3d(sceneBounds.max.x + spacing,
                                                    sceneBounds.max.y + spacing,
                                                    sceneBounds.max.z + spacing)
                                       : Position3d(spacing, spacing, spacing);
    Position3d extent = maxWs - minWs;
    extent.x = std::max(0.0f, extent.x);
    extent.y = std::max(0.0f, extent.y);
    extent.z = std::max(0.0f, extent.z);

    auto countAxis = [&](float extent, float spacing) {
        if (!std::isfinite(extent) || extent <= 0.0f || spacing <= 0.0f) {
            return 1;
        }
        int n = static_cast<int>(std::floor(extent / spacing)) + 1;
        return std::max(1, n);
    };

    int Nx = std::clamp(countAxis(extent.x, spacing), 1, 16);
    int Ny = std::clamp(countAxis(extent.y, spacing), 1, 16);
    int Nz = std::clamp(countAxis(extent.z, spacing), 1, 16);

    probeSpace->originWorldSpace = minWs;
    probeSpace->spacing = Position3d(spacing, spacing, spacing);
    probeSpace->probeCount = Vector3((float)Nx, (float)Ny, (float)Nz);

    probeSpace->debugColor = Color(0.0f, 1.0f, 1.0f);

    const int atlasW = std::max(1, probeSpace->atlasWidth());
    const int atlasH = std::max(1, probeSpace->atlasHeight());
    bool needCreate = !irradianceMap;
    bool sizeChanged =
        !needCreate && (irradianceMap->creationData.width != atlasW ||
                        irradianceMap->creationData.height != atlasH);
    bool canRecreate = !needCreate && irradianceMap->object == nullptr;

    if (needCreate || (sizeChanged && canRecreate)) {
        irradianceMap = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
    }
}

void photon::GlobalIllumination::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) const {
    if (commandBuffer == nullptr || giPipeline == nullptr ||
        irradianceMap == nullptr || irradianceMap->texture == nullptr) {
        return;
    }
    if (irradianceMap->id == 0) {
        irradianceMap->id = irradianceMap->texture->textureID;
    }
    giPipeline->bindTexture("outTexture", irradianceMap->texture, 0);

    giPipeline->setUniform3f("ps.origin", probeSpace->originWorldSpace.x,
                             probeSpace->originWorldSpace.y,
                             probeSpace->originWorldSpace.z);
    giPipeline->setUniform3f("ps.spacing", probeSpace->spacing.x,
                             probeSpace->spacing.y, probeSpace->spacing.z);
    giPipeline->setUniform3f("ps.probeCount", probeSpace->probeCount.x,
                             probeSpace->probeCount.y,
                             probeSpace->probeCount.z);
    giPipeline->setUniform4f("ps.debugColor", probeSpace->debugColor.r,
                             probeSpace->debugColor.g, probeSpace->debugColor.b,
                             probeSpace->debugColor.a);
    giPipeline->setUniform4f("ps.atlasParams",
                             static_cast<float>(probeSpace->textureBorderSize),
                             static_cast<float>(probeSpace->probeResolution),
                             static_cast<float>(probeSpace->probesPerRow),
                             static_cast<float>(probeSpace->totalProbes()));

    commandBuffer->bindPipeline(giPipeline);
    const int dispatchWidth = std::max(1, irradianceMap->creationData.width);
    const int dispatchHeight = std::max(1, irradianceMap->creationData.height);
    commandBuffer->dispatch(static_cast<uint>(dispatchWidth),
                            static_cast<uint>(dispatchHeight), 1);
}
