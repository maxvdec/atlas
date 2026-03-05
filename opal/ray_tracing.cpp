/*
 ray_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Ray tracing APIs for using with the renderer
 Copyright (c) 2026 Max Van den Eynde
*/

#include <memory>
#include <opal/opal.h>
#ifdef METAL

#include "Metal/Metal.hpp"
#include "metal_state.h"

std::shared_ptr<opal::PrimitiveAccelerationStructure>
opal::PrimitiveAccelerationStructure::create(
    const std::vector<PrimitiveVertex> &vertices,
    const std::vector<uint32_t> &indices) {
    auto blas = std::make_shared<PrimitiveAccelerationStructure>();
    blas->vertices = vertices;
    blas->indices = indices;

    auto &deviceState = metal::deviceState(Device::globalInstance);

    std::shared_ptr<MTL::Buffer> vertexBuffer(
        deviceState.device->newBuffer(vertices.data(),
                                      vertices.size() * sizeof(PrimitiveVertex),
                                      MTL::ResourceStorageModeShared),
        [](MTL::Buffer *b) {
            if (b)
                b->release();
        });

    std::shared_ptr<MTL::Buffer> indexBuffer(
        deviceState.device->newBuffer(indices.data(),
                                      indices.size() * sizeof(uint32_t),
                                      MTL::ResourceStorageModeShared),
        [](MTL::Buffer *b) {
            if (b)
                b->release();
        });

    auto *triDesc =
        MTL::AccelerationStructureTriangleGeometryDescriptor::descriptor();

    triDesc->setVertexBuffer(vertexBuffer.get());
    triDesc->setVertexStride(sizeof(PrimitiveVertex));
    triDesc->setVertexFormat(MTL::AttributeFormatFloat3);
    triDesc->setVertexBufferOffset(offsetof(PrimitiveVertex, position));

    triDesc->setIndexBuffer(indexBuffer.get());
    triDesc->setIndexType(MTL::IndexType::IndexTypeUInt32);
    triDesc->setTriangleCount(indices.size() / 3);

    auto *blasDesc =
        MTL::PrimitiveAccelerationStructureDescriptor::descriptor();
    NS::Array *geoms = NS::Array::array((NS::Object **)&triDesc, 1);
    blasDesc->setGeometryDescriptors(geoms);

    MTL::AccelerationStructureSizes sizes =
        deviceState.device->accelerationStructureSizes(blasDesc);

    blas->asBuffer = Buffer::create(BufferUsage::GeneralPurpose,
                                    sizes.accelerationStructureSize);
    blas->scratch = Buffer::create(BufferUsage::GeneralPurpose,
                                   sizes.buildScratchBufferSize);

    MTL::AccelerationStructure *blasPtr =
        deviceState.device->newAccelerationStructure(
            sizes.accelerationStructureSize);

    blas->blas = blasPtr;
    blas->blasDescriptor = blasDesc;

    return blas;
}

void opal::CommandBuffer::buildPrimitiveAccelerationStructure(
    const std::shared_ptr<PrimitiveAccelerationStructure> &blas) {
    auto &deviceState = metal::deviceState(Device::globalInstance);
    auto &scratchBuffer = metal::bufferState(blas->scratch.get());
    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }
    auto *cs = state.commandBuffer;

    auto *asEnc = cs->accelerationStructureCommandEncoder();
    asEnc->buildAccelerationStructure(blas->blas, blas->blasDescriptor,
                                      scratchBuffer.buffer, 0);
    asEnc->endEncoding();
}

#endif
