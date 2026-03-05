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

    pathTracingTexture = std::make_shared<Texture>(
        Texture::create(800, 600, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));
}

void photon::PathTracing::buildAccelerationStructure(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    auto renderables = Window::mainWindow->renderables;
    std::vector<opal::PrimitiveVertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto &renderable : renderables) {
        if (auto *obj = dynamic_cast<CoreObject *>(renderable)) {
            auto objectVertices = obj->vertices;
            auto indices = obj->indices;

            // Append to global lists
            indices.insert(indices.end(), indices.begin(), indices.end());

            for (auto vertex : objectVertices) {
                opal::PrimitiveVertex primVertex;
                primVertex.position[0] = vertex.position.x;
                primVertex.position[1] = vertex.position.y;
                primVertex.position[2] = vertex.position.z;
                primVertex.normal[0] = vertex.normal.x;
                primVertex.normal[1] = vertex.normal.y;
                primVertex.normal[2] = vertex.normal.z;
                primVertex.uv[0] = vertex.textureCoordinate[0];
                primVertex.uv[1] = vertex.textureCoordinate[1];
                primVertex.tangent[0] = vertex.tangent.x;
                primVertex.tangent[1] = vertex.tangent.y;
                primVertex.tangent[2] = vertex.tangent.z;
                primVertex.bitangent[0] = vertex.bitangent.x;
                primVertex.bitangent[1] = vertex.bitangent.y;
                primVertex.bitangent[2] = vertex.bitangent.z;
                vertices.push_back(primVertex);
            }
        }
    }

    this->sceneBLAS =
        opal::PrimitiveAccelerationStructure::create(vertices, indices);
    commandBuffer->buildPrimitiveAccelerationStructure(sceneBLAS);
}

void photon::PathTracing::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    this->buildAccelerationStructure(commandBuffer);
    commandBuffer->bindPipeline(this->pathTracingPipeline);
    pathTracingPipeline->bindTexture("outTex", pathTracingTexture->texture, 0);
    commandBuffer->bindAccelerationStructure(this->sceneBLAS, 0);
    commandBuffer->dispatch((800 + 7) / 8, (600 + 7) / 8, 1);
}
