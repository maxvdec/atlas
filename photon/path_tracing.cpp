/*
 path_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Path Tracing implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include "atlas/window.h"
#include "photon/illuminate.h"
#include "opal/opal.h"

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
    auto renderables = Window::mainWindow->renderables;
    std::vector<opal::PrimitiveVertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto &renderable : renderables) {
        if (auto *obj = dynamic_cast<CoreObject *>(renderable)) {
            const auto &objectVertices = obj->vertices;
            const auto &objectIndices = obj->indices;

            uint32_t baseVertex = (uint32_t)vertices.size();

            vertices.reserve(vertices.size() + objectVertices.size());
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

            indices.reserve(indices.size() + objectIndices.size());
            for (uint32_t i : objectIndices) {
                indices.push_back(baseVertex + i);
            }
        }
    }

    this->sceneBLAS =
        opal::PrimitiveAccelerationStructure::create(vertices, indices);
    commandBuffer->buildPrimitiveAccelerationStructure(sceneBLAS);
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
    commandBuffer->bindPrimitiveAccelerationStructure(this->sceneBLAS, 0);
    commandBuffer->dispatch(Window::mainWindow->viewportWidth,
                            Window::mainWindow->viewportHeight, 1);
}
