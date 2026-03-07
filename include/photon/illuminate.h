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
#include <unordered_map>
#include <utility>
#include <vector>

class Window;
class CoreObject;

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

class PathTracing {
  public:
    void render(const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    void buildAccelerationStructure(
        const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    void createLightBuffers();
    void init();
    void resizeOutput(int width, int height);

    std::shared_ptr<Texture> pathTracingTexture;
    std::shared_ptr<Texture> pathTracingTexturePrev;

    int raysPerPixel = 4;
    int maxBounces = 1;
    float indirectStrength = 0.55f;

    std::shared_ptr<opal::Framebuffer> copySrcFramebuffer;
    std::shared_ptr<opal::Framebuffer> copyDstFramebuffer;

  private:
    std::shared_ptr<opal::Buffer> pointLights;
    std::shared_ptr<opal::Buffer> spotLights;
    std::shared_ptr<opal::Buffer> areaLights;

    std::shared_ptr<opal::Buffer> globalVertices;
    std::shared_ptr<opal::Buffer> globalIndices;
    std::shared_ptr<opal::Buffer> meshInfo;
    std::shared_ptr<opal::Buffer> materialBuffer;
    std::shared_ptr<opal::Buffer> instanceDataBuffer;
    std::vector<std::shared_ptr<opal::Texture>> materialTextures;
    std::shared_ptr<opal::InstanceAccelerationStructure> sceneTLAS;
    std::shared_ptr<opal::Pipeline> pathTracingPipeline;
    std::shared_ptr<ShaderProgram> computePathTracer;
    std::unordered_map<int,
                       std::shared_ptr<opal::PrimitiveAccelerationStructure>>
        objectBLAS;
    std::vector<CoreObject *> cachedObjects;

    int frameIndex = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    glm::mat4 cachedInvViewProj = glm::mat4(1.0f);

    friend class Window;
};

class GlobalIllumination {
  public:
    void render(const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    void updateProbeLayout();
    void init();

    std::shared_ptr<Texture> irradianceMap;
    std::shared_ptr<Texture> irradianceMapPrev;
    std::shared_ptr<ShaderProgram> giWriteShader;
    std::shared_ptr<ShaderProgram> giRaytracingShader;
    std::shared_ptr<opal::Pipeline> giPipeline;
    std::shared_ptr<opal::Pipeline> giRaytracingPipeline;

    std::shared_ptr<opal::Buffer> probeRadianceBuffer;
    int probeRadianceCapacity = 0;

    std::shared_ptr<ProbeSpace> probeSpace;

    std::shared_ptr<opal::Framebuffer> copySrcFramebuffer;
    std::shared_ptr<opal::Framebuffer> copyDstFramebuffer;

    struct DDGIMaterial {
        int materialID;
        int albedoTextureIndex;
        int normalTextureIndex;
        int metallicTextureIndex;

        int roughnessTextureIndex;
        int aoTextureIndex;
        float metallic;
        float roughness;
        float ao;
        float emissiveIntensity;

        glm::vec3 albedo;
        float _pad0;

        glm::vec3 emissiveColor;
        float _pad1;
    };

    struct DDGITriangle {
        glm::vec4 v0;
        glm::vec4 v1;
        glm::vec4 v2;

        glm::vec4 n0;
        glm::vec4 n1;
        glm::vec4 n2;

        glm::vec4 uv0;
        glm::vec4 uv1;
        glm::vec4 uv2;

        glm::vec4 t0;
        glm::vec4 t1;
        glm::vec4 t2;

        glm::vec4 b0;
        glm::vec4 b1;
        glm::vec4 b2;

        int materialID;

        int pad[3];
    };

    std::vector<DDGITriangle> triangles;
    std::vector<DDGIMaterial> materials;
    std::vector<std::shared_ptr<opal::Texture>> materialTextures;

    float probeSpacing = 0.5f;

    int raysPerProbe = 256;
    float maxRayDistance = 20.f;
    float normalBias = 0.05;
    float hysteresis = 0.85;
    int frameIndex = 0;
};

} // namespace photon

#endif // PHOTON_ILLUMINATE_H
