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
#include "atlas/light.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include "photon/illuminate.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
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
    giRaytracingPipeline->setComputeThreadgroupSize(64, 1, 1);
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
    probeSpace->debugColor = Color(1.0f, 1.0f, 1.0f, 0.0f);
    frameIndex = 0;

    copySrcFramebuffer = opal::Framebuffer::create();
    copyDstFramebuffer = opal::Framebuffer::create();
}

void photon::GlobalIllumination::updateProbeLayout() {
    if (probeSpace == nullptr || Window::mainWindow == nullptr) {
        return;
    }

    triangles.clear();
    materials.clear();

    Position3d previousOrigin = probeSpace->originWorldSpace;
    Position3d previousSpacing = probeSpace->spacing;
    Vector3 previousProbeCount = probeSpace->probeCount;
    int previousProbesPerRow = probeSpace->probesPerRow;
    int previousProbeResolution = probeSpace->probeResolution;
    int previousBorder = probeSpace->textureBorderSize;
    float spacing = std::max(0.001f, probeSpacing);
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    bool hasGeometry = false;

    auto includePoint = [&](const glm::vec3 &p) {
        boundsMin = glm::min(boundsMin, p);
        boundsMax = glm::max(boundsMax, p);
        hasGeometry = true;
    };

    auto renderables = Window::mainWindow->renderables;
    materials.reserve(renderables.size());

    for (auto *renderable : renderables) {
        if (renderable == nullptr || !renderable->canUseDeferredRendering()) {
            continue;
        }
        CoreObject *object = dynamic_cast<CoreObject *>(renderable);
        if (object == nullptr) {
            continue;
        }

        const auto &vertices = object->getVertices();
        if (vertices.size() < 3) {
            continue;
        }

        const auto &indices = object->indices;
        const bool useIndexBuffer = indices.size() >= 3;
        glm::mat4 model(1.0f);
        model = glm::translate(model, object->getPosition().toGlm());
        model *=
            glm::mat4_cast(glm::normalize(object->getRotation().toGlmQuat()));
        model = glm::scale(model, object->getScale().toGlm());
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        DDGIMaterial baseMaterial{};
        baseMaterial.albedo = object->material.albedo.toGlm();
        baseMaterial.ao = object->material.ao;
        baseMaterial.metallic = object->material.metallic;
        baseMaterial.roughness = object->material.roughness;
        bool materialBound = false;
        int materialID = -1;

        auto appendTriangle = [&](const CoreVertex &v0, const CoreVertex &v1,
                                  const CoreVertex &v2) {
            if (!materialBound) {
                materialID = static_cast<int>(materials.size());
                baseMaterial.materialID = materialID;
                materials.push_back(baseMaterial);
                materialBound = true;
            }

            DDGITriangle tri{};
            tri.v0 = model * glm::vec4(v0.position.toGlm(), 1.0f);
            tri.v1 = model * glm::vec4(v1.position.toGlm(), 1.0f);
            tri.v2 = model * glm::vec4(v2.position.toGlm(), 1.0f);
            tri.n0 = glm::vec4(glm::normalize(normalMatrix * v0.normal.toGlm()),
                               0.0f);
            tri.n1 = glm::vec4(glm::normalize(normalMatrix * v1.normal.toGlm()),
                               0.0f);
            tri.n2 = glm::vec4(glm::normalize(normalMatrix * v2.normal.toGlm()),
                               0.0f);
            tri.materialID = materialID;

            triangles.push_back(tri);
            includePoint(glm::vec3(tri.v0));
            includePoint(glm::vec3(tri.v1));
            includePoint(glm::vec3(tri.v2));
        };

        if (useIndexBuffer) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                const size_t i0 = static_cast<size_t>(indices[i]);
                const size_t i1 = static_cast<size_t>(indices[i + 1]);
                const size_t i2 = static_cast<size_t>(indices[i + 2]);
                if (i0 >= vertices.size() || i1 >= vertices.size() ||
                    i2 >= vertices.size()) {
                    continue;
                }
                appendTriangle(vertices[i0], vertices[i1], vertices[i2]);
            }
        } else {
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                appendTriangle(vertices[i], vertices[i + 1], vertices[i + 2]);
            }
        }
    }

    float layoutPad = spacing * 0.25f;
    Position3d minWs = hasGeometry ? Position3d(boundsMin.x - layoutPad,
                                                boundsMin.y - layoutPad,
                                                boundsMin.z - layoutPad)
                                   : Position3d(-spacing, -spacing, -spacing);
    Position3d maxWs = hasGeometry ? Position3d(boundsMax.x + layoutPad,
                                                boundsMax.y + layoutPad,
                                                boundsMax.z + layoutPad)
                                   : Position3d(spacing, spacing, spacing);

    auto snapDown = [&](float v) { return std::floor(v / spacing) * spacing; };
    auto snapUp = [&](float v) { return std::ceil(v / spacing) * spacing; };
    minWs.x = snapDown(minWs.x);
    minWs.y = snapDown(minWs.y);
    minWs.z = snapDown(minWs.z);
    maxWs.x = snapUp(maxWs.x);
    maxWs.y = snapUp(maxWs.y);
    maxWs.z = snapUp(maxWs.z);

    Position3d extent = maxWs - minWs;
    extent.x = std::max(0.0f, extent.x);
    extent.y = std::max(0.0f, extent.y);
    extent.z = std::max(0.0f, extent.z);

    auto countAxis = [&](float axisExtent) {
        if (!std::isfinite(axisExtent) || axisExtent <= 0.0f) {
            return 1;
        }
        int n = static_cast<int>(std::floor(axisExtent / spacing)) + 1;
        return std::max(1, n);
    };

    int Nx = std::clamp(countAxis(extent.x), 1, 32);
    int Ny = std::clamp(countAxis(extent.y), 1, 32);
    int Nz = std::clamp(countAxis(extent.z), 1, 32);
    int totalProbeCount = std::max(1, Nx * Ny * Nz);
    int probesPerRow = std::clamp(
        static_cast<int>(std::ceil(std::sqrt((float)totalProbeCount))), 1, 64);

    probeSpace->originWorldSpace = minWs;
    probeSpace->spacing = Position3d(spacing, spacing, spacing);
    probeSpace->probeCount = Vector3((float)Nx, (float)Ny, (float)Nz);
    probeSpace->probesPerRow = probesPerRow;

    float layoutEpsilon = std::max(1e-3f, spacing * 0.25f);
    auto nearlyEqual = [&](float a, float b) {
        return std::fabs(a - b) <= layoutEpsilon;
    };
    bool layoutChanged =
        !nearlyEqual(previousOrigin.x, probeSpace->originWorldSpace.x) ||
        !nearlyEqual(previousOrigin.y, probeSpace->originWorldSpace.y) ||
        !nearlyEqual(previousOrigin.z, probeSpace->originWorldSpace.z) ||
        !nearlyEqual(previousSpacing.x, probeSpace->spacing.x) ||
        !nearlyEqual(previousSpacing.y, probeSpace->spacing.y) ||
        !nearlyEqual(previousSpacing.z, probeSpace->spacing.z) ||
        !nearlyEqual(previousProbeCount.x, probeSpace->probeCount.x) ||
        !nearlyEqual(previousProbeCount.y, probeSpace->probeCount.y) ||
        !nearlyEqual(previousProbeCount.z, probeSpace->probeCount.z) ||
        previousProbesPerRow != probeSpace->probesPerRow ||
        previousProbeResolution != probeSpace->probeResolution ||
        previousBorder != probeSpace->textureBorderSize;

    const int atlasW = std::max(1, probeSpace->atlasWidth());
    const int atlasH = std::max(1, probeSpace->atlasHeight());
    bool needCreate = !irradianceMap || !irradianceMapPrev;
    bool sizeChanged =
        !needCreate && (irradianceMap->creationData.width != atlasW ||
                        irradianceMap->creationData.height != atlasH);
    bool resetHistory = layoutChanged || needCreate || sizeChanged;

    if (needCreate || sizeChanged) {
        irradianceMap = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
        irradianceMapPrev = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
    }

    int effectiveRaysPerProbe = std::max(1, raysPerProbe);
    if (probeRadianceBuffer == nullptr ||
        probeRadianceCapacity != totalProbeCount * effectiveRaysPerProbe) {
        int totalElements = totalProbeCount * effectiveRaysPerProbe;
        probeRadianceBuffer =
            opal::Buffer::create(opal::BufferUsage::ShaderReadWrite,
                                 sizeof(glm::vec4) * totalElements);
        probeRadianceCapacity = totalElements;
        resetHistory = true;
    }

    if (resetHistory) {
        frameIndex = 0;
    }
}

void photon::GlobalIllumination::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    if (commandBuffer == nullptr || giPipeline == nullptr ||
        giRaytracingPipeline == nullptr || probeSpace == nullptr ||
        probeRadianceBuffer == nullptr || irradianceMap == nullptr ||
        irradianceMapPrev == nullptr || irradianceMap->texture == nullptr ||
        irradianceMapPrev->texture == nullptr ||
        copySrcFramebuffer == nullptr || copyDstFramebuffer == nullptr) {
        return;
    }

    const uint totalProbes =
        static_cast<uint>(std::max(1, this->probeSpace->totalProbes()));
    const uint effectiveRays =
        static_cast<uint>(std::max(1, this->raysPerProbe));
    const uint totalRays = totalProbes * effectiveRays;

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
    Scene *scene = (Window::mainWindow != nullptr)
                       ? Window::mainWindow->currentScene
                       : nullptr;
    std::vector<GPUDirectionalLight> directionalLights;
    std::vector<GPUPointLight> pointLights;
    std::vector<GPUSpotLight> spotLights;
    std::vector<GPUAreaLight> areaLights;

    if (scene != nullptr) {
        const auto &sceneDirectionalLights = scene->getDirectionalLights();
        directionalLights.reserve(
            std::min<size_t>(sceneDirectionalLights.size(), 64));
        for (auto *light : sceneDirectionalLights) {
            if (light == nullptr) {
                continue;
            }
            GPUDirectionalLight gpu{};
            gpu.direction = light->direction.toGlm();
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.intensity = light->intensity;
            directionalLights.push_back(gpu);
            if (directionalLights.size() >= 64) {
                break;
            }
        }

        const auto &scenePointLights = scene->getPointLights();
        pointLights.reserve(std::min<size_t>(scenePointLights.size(), 64));
        for (auto *light : scenePointLights) {
            if (light == nullptr) {
                continue;
            }
            PointLightConstants plc = light->calculateConstants();
            GPUPointLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.intensity = light->intensity;
            gpu.constant = plc.constant;
            gpu.linear = plc.linear;
            gpu.quadratic = plc.quadratic;
            gpu.radius = plc.radius;
            pointLights.push_back(gpu);
            if (pointLights.size() >= 64) {
                break;
            }
        }

        const auto &sceneSpotLights = scene->getSpotlights();
        spotLights.reserve(std::min<size_t>(sceneSpotLights.size(), 64));
        for (auto *light : sceneSpotLights) {
            if (light == nullptr) {
                continue;
            }
            GPUSpotLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.direction = light->direction.toGlm();
            gpu.cutOff = light->cutOff;
            gpu.outerCutOff = light->outerCutoff;
            gpu.intensity = light->intensity;
            gpu.range = light->range;
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            spotLights.push_back(gpu);
            if (spotLights.size() >= 64) {
                break;
            }
        }

        const auto &sceneAreaLights = scene->getAreaLights();
        areaLights.reserve(std::min<size_t>(sceneAreaLights.size(), 64));
        for (auto *light : sceneAreaLights) {
            if (light == nullptr) {
                continue;
            }
            GPUAreaLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.right = light->right.toGlm();
            gpu.up = light->up.toGlm();
            gpu.size =
                glm::vec2((float)light->size.width, (float)light->size.height);
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.angle = light->angle;
            gpu.castsBothSides = light->castsBothSides ? 1 : 0;
            gpu.intensity = light->intensity;
            gpu.range = light->range;
            areaLights.push_back(gpu);
            if (areaLights.size() >= 64) {
                break;
            }
        }
    }

    GPUDirectionalLight fallbackDirectional{};
    GPUPointLight fallbackPoint{};
    GPUSpotLight fallbackSpot{};
    GPUAreaLight fallbackArea{};
    giRaytracingPipeline->bindBufferData(
        "directionalLights",
        directionalLights.empty()
            ? static_cast<const void *>(&fallbackDirectional)
            : static_cast<const void *>(directionalLights.data()),
        directionalLights.empty()
            ? sizeof(GPUDirectionalLight)
            : directionalLights.size() * sizeof(GPUDirectionalLight));
    giRaytracingPipeline->bindBufferData(
        "pointLights",
        pointLights.empty() ? static_cast<const void *>(&fallbackPoint)
                            : static_cast<const void *>(pointLights.data()),
        pointLights.empty() ? sizeof(GPUPointLight)
                            : pointLights.size() * sizeof(GPUPointLight));
    giRaytracingPipeline->bindBufferData(
        "spotLights",
        spotLights.empty() ? static_cast<const void *>(&fallbackSpot)
                           : static_cast<const void *>(spotLights.data()),
        spotLights.empty() ? sizeof(GPUSpotLight)
                           : spotLights.size() * sizeof(GPUSpotLight));
    giRaytracingPipeline->bindBufferData(
        "areaLights",
        areaLights.empty() ? static_cast<const void *>(&fallbackArea)
                           : static_cast<const void *>(areaLights.data()),
        areaLights.empty() ? sizeof(GPUAreaLight)
                           : areaLights.size() * sizeof(GPUAreaLight));

    DDGITriangle fallbackTriangle{};
    DDGIMaterial fallbackMaterial{};
    const void *triangleData =
        triangles.empty() ? static_cast<const void *>(&fallbackTriangle)
                          : static_cast<const void *>(triangles.data());
    const size_t triangleSize = triangles.empty()
                                    ? sizeof(DDGITriangle)
                                    : triangles.size() * sizeof(DDGITriangle);
    const void *materialData =
        materials.empty() ? static_cast<const void *>(&fallbackMaterial)
                          : static_cast<const void *>(materials.data());
    const size_t materialSize = materials.empty()
                                    ? sizeof(DDGIMaterial)
                                    : materials.size() * sizeof(DDGIMaterial);
    giRaytracingPipeline->bindBufferData("tris", triangleData, triangleSize);
    giRaytracingPipeline->bindBufferData("materials", materialData,
                                         materialSize);

    struct GPUSceneCounts {
        uint32_t triCount;
        uint32_t materialCount;
        uint32_t directionalLightCount;
        uint32_t pointLightCount;
        uint32_t spotLightCount;
        uint32_t areaLightCount;
        uint32_t _pad0;
        uint32_t _pad1;
    };
    GPUSceneCounts sceneCounts{};
    sceneCounts.triCount = static_cast<uint32_t>(triangles.size());
    sceneCounts.materialCount = static_cast<uint32_t>(materials.size());
    sceneCounts.directionalLightCount =
        static_cast<uint32_t>(directionalLights.size());
    sceneCounts.pointLightCount = static_cast<uint32_t>(pointLights.size());
    sceneCounts.spotLightCount = static_cast<uint32_t>(spotLights.size());
    sceneCounts.areaLightCount = static_cast<uint32_t>(areaLights.size());
    giRaytracingPipeline->bindBufferData("sc", &sceneCounts,
                                         sizeof(sceneCounts));

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
    int ddgiFrameIndex = std::max(0, frameIndex);
    giRaytracingPipeline->setUniform1i("rt.frameIndex", ddgiFrameIndex);

    commandBuffer->bindPipeline(giRaytracingPipeline);
    commandBuffer->dispatch(totalRays, 1, 1);

    commandBuffer->computeBarrier();

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
    giPipeline->setUniform1i("rt.frameIndex", ddgiFrameIndex);

    commandBuffer->bindPipeline(giPipeline);
    const int dispatchWidth = std::max(1, irradianceMap->creationData.width);
    const int dispatchHeight = std::max(1, irradianceMap->creationData.height);
    commandBuffer->dispatch(static_cast<uint>(dispatchWidth),
                            static_cast<uint>(dispatchHeight), 1);

    frameIndex = std::min(frameIndex + 1, 1 << 30);
}
