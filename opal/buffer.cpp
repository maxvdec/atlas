//
// buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Core buffer definitions for allocating memory
// Copyright (c) 2025 maxvdec
//

#include "atlas/tracer/data.h"
#include <memory>
#include <opal/opal.h>
#include <stdexcept>
#include <string>

namespace opal {

namespace {
#ifdef OPENGL
uint getGLVertexAttributeType(VertexAttributeType type) {
    switch (type) {
    case VertexAttributeType::Float:
        return GL_FLOAT;
    case VertexAttributeType::Double:
        return GL_DOUBLE;
    case VertexAttributeType::Int:
        return GL_INT;
    case VertexAttributeType::UnsignedInt:
        return GL_UNSIGNED_INT;
    case VertexAttributeType::Short:
        return GL_SHORT;
    case VertexAttributeType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case VertexAttributeType::Byte:
        return GL_BYTE;
    case VertexAttributeType::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    default:
        return GL_FLOAT;
    }
}

#endif

} // namespace

std::shared_ptr<Buffer> Buffer::create(BufferUsage usage, size_t size,
                                       const void *data,
                                       MemoryUsageType memoryUsage,
                                       int callerId) {
    auto buffer = std::make_shared<Buffer>();
    buffer->usage = usage;
    buffer->memoryUsage = memoryUsage;
#ifdef OPENGL
    glGenBuffers(1, &buffer->bufferID);
    uint glTarget;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::IndexArray:
        glTarget = GL_ELEMENT_ARRAY_BUFFER;
        break;
    case BufferUsage::GeneralPurpose:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::UniformBuffer:
        throw std::runtime_error(
            "UniformBuffer is not support for the OpenGL backend in this "
            "version.");
    case BufferUsage::ShaderRead:
        throw std::runtime_error("ShaderRead is not support for the OpenGL "
                                 "backend in this version.");
    case BufferUsage::ShaderReadWrite:
        glTarget = GL_ARRAY_BUFFER;
        break;
    default:
        glTarget = GL_ARRAY_BUFFER;
        break;
    }
    glBindBuffer(glTarget, buffer->bufferID);
    glBufferData(glTarget, size, data, GL_STATIC_DRAW);
    glBindBuffer(glTarget, 0);
#elif defined(VULKAN)
    auto bufferSize = static_cast<VkDeviceSize>(size);
    VkBufferUsageFlags usageFlags = 0;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::IndexArray:
        usageFlags =
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::GeneralPurpose:
        usageFlags =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::UniformBuffer:
        usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::ShaderRead:
        usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::ShaderReadWrite:
        usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    default:
        usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    }

    VkMemoryPropertyFlags properties = 0;
    switch (memoryUsage) {
    case MemoryUsageType::GPUOnly:
        properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case MemoryUsageType::CPUToGPU:
    case MemoryUsageType::GPUToCPU:
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    default:
        properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    }

    // Store staging buffer in the buffer object so updateData can use it
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 buffer->stagingBuffer, buffer->vkStagingBufferMemory);

    if (data != nullptr) {
        void *mappedData;
        vkMapMemory(Device::globalInstance->logicalDevice,
                    buffer->vkStagingBufferMemory, 0, bufferSize, 0,
                    &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(bufferSize));
        vkUnmapMemory(Device::globalInstance->logicalDevice,
                      buffer->vkStagingBufferMemory);
    }

    createBuffer(bufferSize, usageFlags, properties, buffer->vkBuffer,
                 buffer->vkBufferMemory);
    if (data != nullptr) {
        Buffer::copyBuffer(buffer->stagingBuffer, buffer->vkBuffer, bufferSize);
    }

#endif

    ResourceEventInfo info;
    info.resourceType = DebugResourceType::Buffer;
    info.operation = DebugResourceOperation::Created;
    info.callerObject = std::to_string(callerId);
    info.frameNumber = Device::globalInstance->frameCount;
    info.sizeMb = static_cast<float>(size) / (1024.0f * 1024.0f);
    info.send();

    return buffer;
}

void Buffer::updateData(size_t offset, size_t size, const void *data) {
#ifdef OPENGL
    uint glTarget;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::IndexArray:
        glTarget = GL_ELEMENT_ARRAY_BUFFER;
        break;
    case BufferUsage::GeneralPurpose:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::UniformBuffer:
        throw std::runtime_error(
            "UniformBuffer is not support for the OpenGL backend in this "
            "version.");
    case BufferUsage::ShaderRead:
        throw std::runtime_error("ShaderRead is not support for the OpenGL "
                                 "backend in this version.");
    case BufferUsage::ShaderReadWrite:
        glTarget = GL_ARRAY_BUFFER;
        break;
    default:
        glTarget = GL_ARRAY_BUFFER;
        break;
    }
    glBindBuffer(glTarget, bufferID);
    glBufferSubData(glTarget, offset, size, data);
    glBindBuffer(glTarget, 0);
#elif defined(VULKAN)
    if (vkStagingBufferMemory == VK_NULL_HANDLE ||
        stagingBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error(
            "Buffer::updateData: staging buffer not initialized");
    }
    void *mappedData;
    VkResult result =
        vkMapMemory(Device::globalInstance->logicalDevice,
                    vkStagingBufferMemory, offset, size, 0, &mappedData);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Buffer::updateData: failed to map staging buffer memory");
    }
    memcpy(mappedData, data, size);
    vkUnmapMemory(Device::globalInstance->logicalDevice, vkStagingBufferMemory);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Device::globalInstance->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Device::globalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = offset;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, vkBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(Device::globalInstance->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(Device::globalInstance->graphicsQueue);

    vkFreeCommandBuffers(Device::globalDevice,
                         Device::globalInstance->commandPool, 1,
                         &commandBuffer);
#endif
}

void Buffer::bind(int callerId) const {
#ifdef OPENGL
    uint glTarget;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::IndexArray:
        glTarget = GL_ELEMENT_ARRAY_BUFFER;
        break;
    case BufferUsage::GeneralPurpose:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::UniformBuffer:
        throw std::runtime_error(
            "UniformBuffer is not support for the OpenGL backend in this "
            "version.");
    case BufferUsage::ShaderRead:
        throw std::runtime_error("ShaderRead is not support for the OpenGL "
                                 "backend in this version.");
    case BufferUsage::ShaderReadWrite:
        glTarget = GL_ARRAY_BUFFER;
        break;
    default:
        glTarget = GL_ARRAY_BUFFER;
        break;
    }
    glBindBuffer(glTarget, bufferID);
#endif

    ResourceEventInfo info;
    info.resourceType = DebugResourceType::Buffer;
    info.operation = DebugResourceOperation::Loaded;
    info.callerObject = std::to_string(callerId);
    info.frameNumber = Device::globalInstance->frameCount;
    info.send();
}

void Buffer::unbind(int callerId) const {
#ifdef OPENGL
    uint glTarget;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::IndexArray:
        glTarget = GL_ELEMENT_ARRAY_BUFFER;
        break;
    case BufferUsage::GeneralPurpose:
        glTarget = GL_ARRAY_BUFFER;
        break;
    case BufferUsage::UniformBuffer:
        throw std::runtime_error(
            "UniformBuffer is not support for the OpenGL backend in this "
            "version.");
    case BufferUsage::ShaderRead:
        throw std::runtime_error("ShaderRead is not support for the OpenGL "
                                 "backend in this version.");
    case BufferUsage::ShaderReadWrite:
        glTarget = GL_ARRAY_BUFFER;
        break;
    default:
        glTarget = GL_ARRAY_BUFFER;
        break;
    }
    glBindBuffer(glTarget, 0);
#endif
    ResourceEventInfo info;
    info.resourceType = DebugResourceType::Buffer;
    info.operation = DebugResourceOperation::Unloaded;
    info.callerObject = std::to_string(callerId);
    info.frameNumber = Device::globalInstance->frameCount;
    info.send();
}

std::shared_ptr<DrawingState>
DrawingState::create(std::shared_ptr<Buffer> vertexBuffer,
                     std::shared_ptr<Buffer> indexBuffer) {
    auto state = std::make_shared<DrawingState>();
    state->vertexBuffer = vertexBuffer;
    state->indexBuffer = indexBuffer;

#ifdef OPENGL
    glGenVertexArrays(1, &state->index);
#endif
    return state;
}

void DrawingState::setBuffers(std::shared_ptr<Buffer> vertexBuffer,
                              std::shared_ptr<Buffer> indexBuffer) {
    this->vertexBuffer = vertexBuffer;
    this->indexBuffer = indexBuffer;
}

void DrawingState::bind() const {
#ifdef OPENGL
    glBindVertexArray(index);
    if (vertexBuffer) {
        vertexBuffer->bind();
    }
    if (indexBuffer) {
        indexBuffer->bind();
    }
#endif
}

void DrawingState::unbind() const {
#ifdef OPENGL
    glBindVertexArray(0);
    if (indexBuffer) {
        indexBuffer->unbind();
    }
    if (vertexBuffer) {
        vertexBuffer->unbind();
    }
#endif
}

void DrawingState::configureAttributes(
    const std::vector<VertexAttributeBinding> &bindings) {
#ifdef OPENGL
    if (bindings.empty()) {
        return;
    }

    glBindVertexArray(index);

    for (const auto &binding : bindings) {
        auto buffer =
            binding.sourceBuffer ? binding.sourceBuffer : vertexBuffer;
        if (buffer == nullptr) {
            throw std::runtime_error("No vertex buffer bound for attribute '" +
                                     binding.attribute.name + "'.");
        }

        buffer->bind();
        glEnableVertexAttribArray(binding.attribute.location);
        glVertexAttribPointer(binding.attribute.location,
                              binding.attribute.size,
                              getGLVertexAttributeType(binding.attribute.type),
                              binding.attribute.normalized ? GL_TRUE : GL_FALSE,
                              binding.attribute.stride,
                              reinterpret_cast<void *>(static_cast<uintptr_t>(
                                  binding.attribute.offset)));

        unsigned int divisor = binding.attribute.divisor;
        if (binding.attribute.inputRate == VertexBindingInputRate::Instance) {
            divisor = divisor == 0 ? 1 : divisor;
        }
        glVertexAttribDivisor(binding.attribute.location, divisor);

        if (binding.sourceBuffer) {
            binding.sourceBuffer->unbind();
        }
    }

    glBindVertexArray(0);
#elif defined(VULKAN)
    bool hasInstanceBinding = false;
    for (const auto &binding : bindings) {
        if (binding.attribute.inputRate == VertexBindingInputRate::Instance &&
            binding.sourceBuffer != nullptr) {
            instanceBuffer = binding.sourceBuffer;
            hasInstanceBinding = true;
            break;
        }
    }
    if (!hasInstanceBinding) {
        instanceBuffer = nullptr;
    }
#else
    (void)bindings;
#endif
}

} // namespace opal
