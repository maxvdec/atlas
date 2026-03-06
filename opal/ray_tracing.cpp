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

    blas->vertexBuffer = std::shared_ptr<MTL::Buffer>(
        deviceState.device->newBuffer(vertices.data(),
                                      vertices.size() * sizeof(PrimitiveVertex),
                                      MTL::ResourceStorageModeShared),
        [](MTL::Buffer *b) {
            if (b)
                b->release();
        });

    blas->indexBuffer = std::shared_ptr<MTL::Buffer>(
        deviceState.device->newBuffer(indices.data(),
                                      indices.size() * sizeof(uint32_t),
                                      MTL::ResourceStorageModeShared),
        [](MTL::Buffer *b) {
            if (b)
                b->release();
        });

    auto *triDesc =
        MTL::AccelerationStructureTriangleGeometryDescriptor::descriptor();

    triDesc->setVertexBuffer(blas->vertexBuffer.get());
    triDesc->setVertexStride(sizeof(PrimitiveVertex));
    triDesc->setVertexFormat(MTL::AttributeFormatFloat3);
    triDesc->setVertexBufferOffset(offsetof(PrimitiveVertex, position));

    triDesc->setIndexBuffer(blas->indexBuffer.get());
    triDesc->setIndexType(MTL::IndexType::IndexTypeUInt32);
    triDesc->setTriangleCount(indices.size() / 3);

    auto *blasDesc =
        MTL::PrimitiveAccelerationStructureDescriptor::descriptor()->retain();
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
                                                float out[12]) {
    out[0] = M[0][0];
    out[1] = M[0][1];
    out[2] = M[0][2];
    out[3] = M[1][0];
    out[4] = M[1][1];
    out[5] = M[1][2];
    out[6] = M[2][0];
    out[7] = M[2][1];
    out[8] = M[2][2];
    out[9] = M[3][0];
    out[10] = M[3][1];
    out[11] = M[3][2];
}

std::shared_ptr<opal::InstanceAccelerationStructure>
opal::InstanceAccelerationStructure::create(
    const std::vector<opal::AccelerationStructureInstance> &instances) {
    auto tlas = std::make_shared<opal::InstanceAccelerationStructure>();
    tlas->instances = instances;

    tlas->blasRefs.reserve(instances.size());
    tlas->blasPtrs.reserve(instances.size());

    std::vector<MTL::AccelerationStructureUserIDInstanceDescriptor> descs;
    descs.resize(instances.size());

    for (size_t i = 0; i < instances.size(); ++i) {
        const auto &inst = instances[i];

        if (!inst.blas || !inst.blas->isBuilt)
            throw std::runtime_error("All BLAS must be built before TLAS.");

        tlas->blasRefs.push_back(inst.blas);
        tlas->blasPtrs.push_back(inst.blas->blas);

        auto &d = descs[i];
        memset(&d, 0, sizeof(d));

        float t[12];
        writeMetalTransform3x4(inst.transform, t);

        memcpy(&d.transformationMatrix, t, sizeof(float) * 12);

        d.accelerationStructureIndex = (uint32_t)i;
        d.mask = inst.mask ? inst.mask : 0xFF;
        d.userID = inst.instanceId;
        d.options =
            inst.cullDisable
                ? MTL::AccelerationStructureInstanceOptionDisableTriangleCulling
                : 0;
        d.intersectionFunctionTableOffset = 0;
    }

    tlas->instanceBuffer =
        Buffer::create(BufferUsage::GeneralPurpose,
                       descs.size() * sizeof(descs[0]), descs.data());

    tlas->tlasDescriptor =
        MTL::InstanceAccelerationStructureDescriptor::descriptor()->retain();
    auto &ib = metal::bufferState(tlas->instanceBuffer.get());

    tlas->tlasDescriptor->setInstanceDescriptorBuffer(ib.buffer);
    tlas->tlasDescriptor->setInstanceDescriptorStride(
        sizeof(MTL::AccelerationStructureUserIDInstanceDescriptor));
    tlas->tlasDescriptor->setInstanceCount((NS::UInteger)descs.size());

    NS::Array *instancedAS =
        NS::Array::array((NS::Object **)tlas->blasPtrs.data(),
                         (NS::UInteger)tlas->blasPtrs.size());
    tlas->tlasDescriptor->setInstancedAccelerationStructures(instancedAS);

    metal::DeviceState &deviceState =
        metal::deviceState(Device::globalInstance);

    MTL::AccelerationStructureSizes sizes =
        deviceState.device->accelerationStructureSizes(tlas->tlasDescriptor);

    tlas->scratch = Buffer::create(BufferUsage::GeneralPurpose,
                                   sizes.buildScratchBufferSize);

    MTL::AccelerationStructure *tlasPtr =
        deviceState.device->newAccelerationStructure(
            sizes.accelerationStructureSize);

    tlas->tlas = tlasPtr;

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
    auto &scratchState = metal::bufferState(tlas->scratch.get());

    auto *asEnc = cs->accelerationStructureCommandEncoder();
    asEnc->buildAccelerationStructure(tlas->tlas, tlas->tlasDescriptor,
                                      scratchState.buffer, 0);
    asEnc->endEncoding();

    tlas->isBuilt = true;
}

#endif
