/*
 gi.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: DDGI Implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/component.h"
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
    ComputeShader ddgiRaytracing =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI);
    ddgiRaytracing.compile();
    giRaytracingShader = std::make_shared<ShaderProgram>();
    giRaytracingShader->computeShader = ddgiRaytracing;
    giRaytracingShader->compile();

    giRaytracingPipeline = opal::Pipeline::create();
    giRaytracingPipeline->setShaderProgram(giRaytracingShader->shader);
    giRaytracingPipeline->setComputeThreadgroupSize(8, 8, 1);
    giRaytracingPipeline->build();

    ComputeShader ddgiWrite =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI_WRITE);
    ddgiWrite.compile();
    giWriteShader = std::make_shared<ShaderProgram>();
    giWriteShader->computeShader = ddgiWrite;
    giWriteShader->compile();

    irradianceMap = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    irradianceMapPrev = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    giPipeline = opal::Pipeline::create();
    giPipeline->setShaderProgram(giWriteShader->shader);
    giPipeline->setComputeThreadgroupSize(8, 8, 1);
    giPipeline->build();

    probeSpace = std::make_shared<ProbeSpace>();

    copySrcFramebuffer = opal::Framebuffer::create();
    copyDstFramebuffer = opal::Framebuffer::create();
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

        irradianceMapPrev = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
    }

    if (probeRadianceBuffer == nullptr) {
        probeRadianceBuffer =
            opal::Buffer::create(opal::BufferUsage::ShaderReadWrite,
                                 sizeof(glm::vec3) * probeSpace->totalProbes());
    }

    auto renderables = Window::mainWindow->renderables;

    for (auto *renderable : renderables) {
        if (CoreObject *object = dynamic_cast<CoreObject *>(renderable)) {
            DDGIMaterial mat{};
            mat.albedo = object->material.albedo.toGlm();
            mat.ao = object->material.ao;
            mat.metallic = object->material.metallic;
            mat.roughness = object->material.roughness;
            mat.materialID = static_cast<int>(materials.size());

            materials.push_back(mat);

            const auto &vertices = object->getVertices();
            const auto &indices = object->indices;

            glm::mat4 model = object->model;

            glm::mat3 normalMatrix =
                glm::transpose(glm::inverse(glm::mat3(model)));

            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                const CoreVertex &v0 = vertices[indices[i]];
                const CoreVertex &v1 = vertices[indices[i + 1]];
                const CoreVertex &v2 = vertices[indices[i + 2]];

                DDGITriangle tri{};

                tri.v0 = model * glm::vec4(v0.position.toGlm(), 1.0f);
                tri.v1 = model * glm::vec4(v1.position.toGlm(), 1.0f);
                tri.v2 = model * glm::vec4(v2.position.toGlm(), 1.0f);

                tri.n0 = glm::vec4(
                    glm::normalize(normalMatrix * v0.normal.toGlm()), 0.0f);
                tri.n1 = glm::vec4(
                    glm::normalize(normalMatrix * v1.normal.toGlm()), 0.0f);
                tri.n2 = glm::vec4(
                    glm::normalize(normalMatrix * v2.normal.toGlm()), 0.0f);

                tri.materialID = mat.materialID;

                triangles.push_back(tri);
            }
        }
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

    // Copy the old map
    copySrcFramebuffer->attachTexture(irradianceMap->texture, 0);
    copyDstFramebuffer->attachTexture(irradianceMapPrev->texture, 0);
    auto copy = opal::ResolveAction::createForColorAttachment(
        copySrcFramebuffer, copyDstFramebuffer, 0);
    commandBuffer->performResolve(copy);

    // Perform Ray Tracing
    giRaytracingPipeline->bindShaderReadWriteBuffer("probeRadianceOut",
                                                    probeRadianceBuffer);
    giRaytracingPipeline->bindBufferData(
        "tris", triangles.data(), triangles.size() * sizeof(DDGITriangle));
    giRaytracingPipeline->bindBufferData(
        "materials", materials.data(), materials.size() * sizeof(DDGIMaterial));
    giRaytracingPipeline->setUniform1i("triCount", triangles.size());
    giRaytracingPipeline->setUniform3f(
        "ps.origin", probeSpace->originWorldSpace.x,
        probeSpace->originWorldSpace.y, probeSpace->originWorldSpace.z);
    giRaytracingPipeline->setUniform3f("ps.spacing", probeSpace->spacing.x,
                                       probeSpace->spacing.y,
                                       probeSpace->spacing.z);
    giRaytracingPipeline->setUniform3f(
        "ps.probeCount", probeSpace->probeCount.x, probeSpace->probeCount.y,
        probeSpace->probeCount.z);
    giRaytracingPipeline->setUniform4f(
        "ps.debugColor", probeSpace->debugColor.r, probeSpace->debugColor.g,
        probeSpace->debugColor.b, probeSpace->debugColor.a);
    giRaytracingPipeline->setUniform4f(
        "ps.atlasParams", static_cast<float>(probeSpace->textureBorderSize),
        static_cast<float>(probeSpace->probeResolution),
        static_cast<float>(probeSpace->probesPerRow),
        static_cast<float>(probeSpace->totalProbes()));

    giRaytracingPipeline->setUniform1i("rt.raysPerProbe", this->raysPerProbe);
    giRaytracingPipeline->setUniform1f("rt.maxRayDistance",
                                       this->maxRayDistance);
    giRaytracingPipeline->setUniform1f("rt.normalBias", this->normalBias);
    giRaytracingPipeline->setUniform1f("rt.hysteresis", this->hysteresis);
    giRaytracingPipeline->setUniform1i("rt.frameIndex",
                                       Window::mainWindow->currentFrame);

    commandBuffer->bindPipeline(giRaytracingPipeline);
    commandBuffer->dispatch(static_cast<uint>(this->probeSpace->totalProbes()),
                            1, 1);

    // Write to irradiance texture
    giPipeline->bindTexture("outTexture", irradianceMap->texture, 0);
    giPipeline->bindTexture("prevTexture", irradianceMapPrev->texture, 1);

    giPipeline->bindBuffer("probeRadiance", this->probeRadianceBuffer);

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

    giPipeline->setUniform1i("rt.raysPerProbe", this->raysPerProbe);
    giPipeline->setUniform1f("rt.maxRayDistance", this->maxRayDistance);
    giPipeline->setUniform1f("rt.normalBias", this->normalBias);
    giPipeline->setUniform1f("rt.hysteresis", this->hysteresis);
    giPipeline->setUniform1i("rt.frameIndex", Window::mainWindow->currentFrame);

    commandBuffer->bindPipeline(giPipeline);
    const int dispatchWidth = std::max(1, irradianceMap->creationData.width);
    const int dispatchHeight = std::max(1, irradianceMap->creationData.height);
    commandBuffer->dispatch(static_cast<uint>(dispatchWidth),
                            static_cast<uint>(dispatchHeight), 1);
}
