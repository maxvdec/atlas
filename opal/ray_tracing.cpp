/*
 ray_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Ray tracing APIs for using with the renderer
 Copyright (c) 2026 Max Van den Eynde
*/

#include <cstddef>
#include <cstdint>
#include <cstring>
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

    blas->isBuilt = true;
}

static inline void opal::writeMetalTransform3x4(const glm::mat4 &M,
                                                float out3x4[12]) {
    out3x4[0] = M[0][0];
    out3x4[1] = M[1][0];
    out3x4[2] = M[2][0];
    out3x4[3] = M[3][0];
    out3x4[4] = M[0][1];
    out3x4[5] = M[1][1];
    out3x4[6] = M[2][1];
    out3x4[7] = M[3][1];
    out3x4[8] = M[0][2];
    out3x4[9] = M[1][2];
    out3x4[10] = M[2][2];
    out3x4[11] = M[3][2];
}

std::shared_ptr<opal::InstanceAccelerationStructure>
opal::InstanceAccelerationStructure::create(
    const std::vector<opal::AccelerationStructureInstance> &instances) {
    auto tlas = std::make_shared<opal::InstanceAccelerationStructure>();

    std::vector<MTL::AccelerationStructure *> blasList;
    blasList.reserve(instances.size());

    std::vector<MTL::AccelerationStructureInstanceDescriptor> instanceDescs;
    instanceDescs.reserve(instances.size());

    for (size_t i = 0; i < instances.size(); i++) {
        const auto &instance = instances[i];
        if (instance.blas == nullptr || !instance.blas->isBuilt) {
            throw std::runtime_error(
                "All BLAS instances must be built before creating TLAS");
        }
        blasList.push_back(instance.blas->blas);

        auto &d = instanceDescs[i];
        memset(&d, 0, sizeof(d));

        float t[12];
        writeMetalTransform3x4(instance.transform, t);
        memcpy(d.transformationMatrix.columns, t, sizeof(t));

        d.accelerationStructureIndex = static_cast<uint32_t>(i);
        d.mask = instance.mask;
        d.options =
            instance.cullDisable
                ? MTL::AccelerationStructureInstanceOptionDisableTriangleCulling
                : 0;
        d.intersectionFunctionTableOffset = 0;
        d.mask = instance.mask;
    }

    tlas->instanceBuffer = Buffer::create(BufferUsage::GeneralPurpose,
                                          instances.size(), instances.data());

    return tlas;
}

void opal::CommandBuffer::buildInstanceAccelerationStructure(
    const std::shared_ptr<InstanceAccelerationStructure> &tlas) {
    auto &deviceState = metal::deviceState(Device::globalInstance);
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

    auto &bufferState = metal::bufferState(tlas->instanceBuffer.get());

    auto *asEnc = cs->accelerationStructureCommandEncoder();
    asEnc->buildAccelerationStructure(tlas->tlas, tlas->tlasDescriptor,
                                      bufferState.buffer, 0);
    asEnc->endEncoding();
}

#endif
