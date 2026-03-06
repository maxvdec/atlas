/*
 path_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Path Tracing implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/window.h"
#include "photon/illuminate.h"
#include "opal/opal.h"
#include <cmath>
#include <cstring>
#include <ranges>
#include <sys/types.h>

void photon::PathTracing::init() {
    ComputeShader pathTracerShader =
        ComputeShader::fromDefaultShader(AtlasComputeShader::PathTracer);
    pathTracerShader.compile();
    computePathTracer = std::make_shared<ShaderProgram>();
    computePathTracer->computeShader = pathTracerShader;
    computePathTracer->compile();

    pathTracingPipeline = opal::Pipeline::create();
    pathTracingPipeline->setShaderProgram(computePathTracer->shader);
    pathTracingPipeline->setComputeThreadgroupSize(8, 8, 1);
    pathTracingPipeline->build();

    pathTracingTexture = std::make_shared<Texture>(Texture::create(
        Window::mainWindow->viewportWidth, Window::mainWindow->viewportHeight,
        opal::TextureFormat::Rgba16F, opal::TextureDataFormat::Rgba,
        TextureType::Color));
}

void photon::PathTracing::buildAccelerationStructure(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    struct MaterialData {
        float albedo[4];
        float metallic;
        float roughness;
        float ao;
        uint _pad0;
    };

    struct MeshData {
        uint vertexOffset;
        uint indexOffset;
        uint _pad0;
        uint _pad1;
    };

    struct VertexData {
        float normal[3];
    };

    auto renderables = Window::mainWindow->renderables;
    std::vector<MaterialData> materialData;

    std::vector<VertexData> allVertices;
    std::vector<uint32_t> allIndices;
    std::vector<MeshData> meshData;

    int objectID = 0;
    if (objectBLAS.empty()) {
        for (auto *renderable : renderables) {
            if (auto *object = dynamic_cast<CoreObject *>(renderable)) {
                const auto &objectVertices = object->vertices;
                const auto &objectIndices = object->indices;

                std::vector<opal::PrimitiveVertex> vertices;
                vertices.reserve(objectVertices.size());
                std::vector<uint32_t> indices;
                indices.reserve(objectIndices.size());

                int vertexOffset = allVertices.size();
                int indexOffset = allIndices.size();

                for (const auto &v : objectVertices) {
                    opal::PrimitiveVertex pv{};
                    pv.position[0] = v.position.x;
                    pv.position[1] = v.position.y;
                    pv.position[2] = v.position.z;
                    pv.normal[0] = v.normal.x;
                    pv.normal[1] = v.normal.y;
                    pv.normal[2] = v.normal.z;
                    pv.uv[0] = v.textureCoordinate[0];
                    pv.uv[1] = v.textureCoordinate[1];
                    pv.tangent[0] = v.tangent.x;
                    pv.tangent[1] = v.tangent.y;
                    pv.tangent[2] = v.tangent.z;
                    pv.bitangent[0] = v.bitangent.x;
                    pv.bitangent[1] = v.bitangent.y;
                    pv.bitangent[2] = v.bitangent.z;
                    vertices.push_back(pv);

                    VertexData vd{};
                    vd.normal[0] = v.normal.x;
                    vd.normal[1] = v.normal.y;
                    vd.normal[2] = v.normal.z;
                    allVertices.push_back(vd);
                }

                indices = objectIndices;
                for (auto &index : indices) {
                    allIndices.push_back(vertexOffset + index);
                }

                auto blas = opal::PrimitiveAccelerationStructure::create(
                    vertices, indices);
                objectBLAS[objectID] = blas;

                MaterialData data;
                data.albedo[0] = object->material.albedo.r;
                data.albedo[1] = object->material.albedo.g;
                data.albedo[2] = object->material.albedo.b;
                data.albedo[3] = object->material.albedo.a;
                data.metallic = object->material.metallic;
                data.roughness = object->material.roughness;
                data.ao = object->material.ao;
                materialData.push_back(data);

                MeshData mdata;
                mdata.vertexOffset = vertexOffset;
                mdata.indexOffset = indexOffset;
                meshData.push_back(mdata);

                objectID++;
            }
        }

        for (const auto &[id, blas] : objectBLAS) {
            if (blas != nullptr) {
                commandBuffer->buildPrimitiveAccelerationStructure(blas);
            }
        }

        materialBuffer = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            materialData.size() * sizeof(MaterialData), materialData.data());

        globalVertices = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            allVertices.size() * sizeof(VertexData), allVertices.data());

        globalIndices = opal::Buffer::create(
            opal::BufferUsage::ShaderRead, allIndices.size() * sizeof(uint32_t),
            allIndices.data());

        meshInfo = opal::Buffer::create(opal::BufferUsage::ShaderRead,
                                        meshData.size() * sizeof(MeshData),
                                        meshData.data());
    }

    std::vector<opal::AccelerationStructureInstance> instances;
    objectID = 0;

    struct InstanceData {
        float model[16];
        float normalCol0[4];
        float normalCol1[4];
        float normalCol2[4];
    };

    std::vector<InstanceData> instanceData;

    for (auto *renderable : renderables) {
        if (auto *object = dynamic_cast<CoreObject *>(renderable)) {
            auto it = objectBLAS.find(objectID);
            if (it == objectBLAS.end()) {
                continue;
            }
            auto blas = it->second;
            if (blas == nullptr || !blas->isBuilt) {
                continue;
            }

            opal::AccelerationStructureInstance instance{};
            instance.blas = blas;
            instance.transform = object->model;
            instance.instanceId = objectID;
            instance.mask = 0xFF;
            instance.cullDisable = false;
            instances.push_back(instance);

            InstanceData d{};
            const glm::mat4 &m = object->model;
            memcpy(d.model, &m[0][0], sizeof(float) * 16);
            glm::mat3 linear = glm::mat3(m);
            glm::mat3 normalMatrix(1.0f);
            float det = glm::determinant(linear);
            if (std::fabs(det) > 0.000001f) {
                normalMatrix = glm::transpose(glm::inverse(linear));
            }
            d.normalCol0[0] = normalMatrix[0][0];
            d.normalCol0[1] = normalMatrix[0][1];
            d.normalCol0[2] = normalMatrix[0][2];
            d.normalCol0[3] = 0.0f;
            d.normalCol1[0] = normalMatrix[1][0];
            d.normalCol1[1] = normalMatrix[1][1];
            d.normalCol1[2] = normalMatrix[1][2];
            d.normalCol1[3] = 0.0f;
            d.normalCol2[0] = normalMatrix[2][0];
            d.normalCol2[1] = normalMatrix[2][1];
            d.normalCol2[2] = normalMatrix[2][2];
            d.normalCol2[3] = 0.0f;
            instanceData.push_back(d);

            objectID++;
        }
    }

    instanceDataBuffer = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        instanceData.size() * sizeof(InstanceData), instanceData.data());

    sceneTLAS = opal::InstanceAccelerationStructure::create(instances);
    commandBuffer->buildInstanceAccelerationStructure(sceneTLAS);
}

void photon::PathTracing::createLightBuffers() {
    struct PointLightData {
        float position[3];
        float intensity;
        float color[3];
        float range;
    };

    struct SpotLightData {
        float position[3];
        float intensity;

        float direction[3];
        float innerCos;

        float color[3];
        float outerCos;

        float range;
        float _pad0[3];
    };

    struct AreaLightData {
        float position[3];
        float intensity;

        float right[3];
        float halfWidth;

        float up[3];
        float halfHeight;

        float color[3];
        float twoSided;
    };

    static_assert(sizeof(PointLightData) == 32);
    static_assert(sizeof(SpotLightData) == 64);
    static_assert(sizeof(AreaLightData) == 64);

    std::vector<PointLightData> pointLightData;
    std::vector<SpotLightData> spotLightData;
    std::vector<AreaLightData> areaLightData;

    Scene *scene =
        (Window::mainWindow != nullptr) ? Window::mainWindow->getCurrentScene()
                                        : nullptr;

    if (scene != nullptr) {
        for (auto *light : scene->getPointLights()) {
            if (light == nullptr) {
                continue;
            }
            PointLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.range = light->distance;
            pointLightData.push_back(data);
        }

        for (auto *light : scene->getSpotlights()) {
            if (light == nullptr) {
                continue;
            }
            SpotLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.direction[0] = light->direction.x;
            data.direction[1] = light->direction.y;
            data.direction[2] = light->direction.z;
            data.innerCos = light->cutOff;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.outerCos = light->outerCutoff;
            data.range = light->range;
            spotLightData.push_back(data);
        }

        for (auto *light : scene->getAreaLights()) {
            if (light == nullptr) {
                continue;
            }
            AreaLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.right[0] = light->right.x;
            data.right[1] = light->right.y;
            data.right[2] = light->right.z;
            data.halfWidth = light->size.width * 0.5f;
            data.up[0] = light->up.x;
            data.up[1] = light->up.y;
            data.up[2] = light->up.z;
            data.halfHeight = light->size.height * 0.5f;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.twoSided = light->castsBothSides ? 1.0f : 0.0f;
            areaLightData.push_back(data);
        }
    }

    if (pointLightData.empty()) {
        pointLightData.push_back(PointLightData{});
    }
    this->pointLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        pointLightData.size() * sizeof(PointLightData), pointLightData.data());

    if (spotLightData.empty()) {
        spotLightData.push_back(SpotLightData{});
    }
    this->spotLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        spotLightData.size() * sizeof(SpotLightData), spotLightData.data());

    if (areaLightData.empty()) {
        areaLightData.push_back(AreaLightData{});
    }
    this->areaLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        areaLightData.size() * sizeof(AreaLightData), areaLightData.data());
}

void photon::PathTracing::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {

    auto view = Window::mainWindow->getCamera()->calculateViewMatrix();
    auto proj = Window::mainWindow->calculateProjectionMatrix();

    auto invViewProj = glm::inverse(proj * view);

    pathTracingPipeline->setUniformMat4f("cam.invViewProj", invViewProj);
    pathTracingPipeline->setUniform3f(
        "cam.camPos", Window::mainWindow->getCamera()->position.x,
        Window::mainWindow->getCamera()->position.y,
        Window::mainWindow->getCamera()->position.z);

    int directionalLightCount = 0;
    int pointLightCount = 0;
    int spotLightCount = 0;
    int areaLightCount = 0;

    Scene *scene =
        (Window::mainWindow != nullptr) ? Window::mainWindow->getCurrentScene()
                                        : nullptr;

    if (scene != nullptr) {
        const auto &directionalLights = scene->getDirectionalLights();
        for (auto *light : directionalLights) {
            if (light == nullptr) {
                continue;
            }
            glm::vec3 directionalLightDirection(0.0f, -1.0f, 0.0f);
            glm::vec3 sceneDirection = light->direction.toGlm();
            if (glm::length(sceneDirection) > 0.0001f) {
                directionalLightDirection = glm::normalize(sceneDirection);
            }
            glm::vec3 directionalLightColor =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            float directionalLightIntensity = light->intensity;

            pathTracingPipeline->setUniform3f(
                "dirLight.direction", directionalLightDirection.x,
                directionalLightDirection.y, directionalLightDirection.z);
            pathTracingPipeline->setUniform3f(
                "dirLight.color", directionalLightColor.x,
                directionalLightColor.y, directionalLightColor.z);
            pathTracingPipeline->setUniform1f("dirLight.intensity",
                                              directionalLightIntensity);
            directionalLightCount = 1;
            break;
        }

        for (auto *light : scene->getPointLights()) {
            if (light != nullptr) {
                pointLightCount++;
            }
        }
        for (auto *light : scene->getSpotlights()) {
            if (light != nullptr) {
                spotLightCount++;
            }
        }
        for (auto *light : scene->getAreaLights()) {
            if (light != nullptr) {
                areaLightCount++;
            }
        }
    }

    pathTracingPipeline->setUniform1i("sceneData.numDirectionalLights",
                                      directionalLightCount);
    pathTracingPipeline->setUniform1i("sceneData.numPointLights",
                                      pointLightCount);
    pathTracingPipeline->setUniform1i("sceneData.numSpotLights",
                                      spotLightCount);
    pathTracingPipeline->setUniform1i("sceneData.numAreaLights",
                                      areaLightCount);

    this->buildAccelerationStructure(commandBuffer);
    this->createLightBuffers();
    commandBuffer->bindPipeline(this->pathTracingPipeline);
    pathTracingPipeline->bindTexture("outTex", pathTracingTexture->texture, 0);
    commandBuffer->bindInstanceAccelerationStructure(this->sceneTLAS, 0);

    pathTracingPipeline->bindBuffer("materials", materialBuffer, 2);
    pathTracingPipeline->bindBuffer("meshData", meshInfo, 3);
    pathTracingPipeline->bindBuffer("vertices", globalVertices, 4);
    pathTracingPipeline->bindBuffer("indices", globalIndices, 5);
    pathTracingPipeline->bindBuffer("instanceData", instanceDataBuffer, 6);
    pathTracingPipeline->bindBuffer("pointLights", pointLights, 9);
    pathTracingPipeline->bindBuffer("spotLights", spotLights, 10);
    pathTracingPipeline->bindBuffer("areaLights", areaLights, 11);

    commandBuffer->dispatch(Window::mainWindow->viewportWidth,
                            Window::mainWindow->viewportHeight, 1);
}
