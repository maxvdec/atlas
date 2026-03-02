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
#include "opal/opal.h"
#include "photon/illuminate.h"
#include <memory>

void photon::GlobalIllumination::init() {
    ComputeShader ddgiShader =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI);
    ddgiShader.compile();
    giShader = std::make_shared<ShaderProgram>();
    giShader->computeShader = ddgiShader;
    giShader->compile();

    irradianceMap = std::make_shared<Texture>(Texture::create(
        512, 512, opal::TextureFormat::Rgba16F, opal::TextureDataFormat::Rgba,
        TextureType::Color));

    giPipeline = opal::Pipeline::create();
    giPipeline->setShaderProgram(giShader->shader);
    giPipeline->setComputeThreadgroupSize(8, 8, 1);
    giPipeline->build();
}

void photon::GlobalIllumination::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) const {
    if (irradianceMap == nullptr || irradianceMap->texture == nullptr) {
        return;
    }
    if (irradianceMap->id == 0) {
        irradianceMap->id = irradianceMap->texture->textureID;
    }
    giPipeline->bindTexture("outTexture", irradianceMap->texture, 0);

    commandBuffer->bindPipeline(giPipeline);
    commandBuffer->dispatch(512, 512);
}
