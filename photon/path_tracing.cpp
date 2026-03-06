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

    auto renderables = Window::mainWindow->renderables;
    std::vector<MaterialData> materialData;

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
                }

                indices = objectIndices;

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
    }

    std::vector<opal::AccelerationStructureInstance> instances;
    objectID = 0;
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

            objectID++;
        }
    }

    sceneTLAS = opal::InstanceAccelerationStructure::create(instances);
    commandBuffer->buildInstanceAccelerationStructure(sceneTLAS);
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

    this->buildAccelerationStructure(commandBuffer);
    commandBuffer->bindPipeline(this->pathTracingPipeline);
    pathTracingPipeline->bindTexture("outTex", pathTracingTexture->texture, 0);
    commandBuffer->bindInstanceAccelerationStructure(this->sceneTLAS, 0);

    pathTracingPipeline->bindBuffer("materials", materialBuffer, 2);
    commandBuffer->dispatch(Window::mainWindow->viewportWidth,
                            Window::mainWindow->viewportHeight, 1);
}
