//
// buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Core buffer definitions for allocating memory
// Copyright (c) 2025 maxvdec
//

#include <memory>
#include <opal/opal.h>
#include <stdexcept>
#include <string>

namespace opal {

namespace {

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

} // namespace

std::shared_ptr<Buffer> Buffer::create(BufferUsage usage, size_t size,
                                       const void *data,
                                       MemoryUsageType memoryUsage) {
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
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    switch (usage) {
    case BufferUsage::VertexBuffer:
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    case BufferUsage::IndexArray:
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case BufferUsage::GeneralPurpose:
        bufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferUsage::UniformBuffer:
    case BufferUsage::ShaderRead:
    case BufferUsage::ShaderReadWrite:
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;
    default:
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    }

    switch (memoryUsage) {
    case MemoryUsageType::GPUOnly:
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        break;
    case MemoryUsageType::CPUToGPU:
    case MemoryUsageType::GPUToCPU:
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        break;
    }

    if (vkCreateBuffer(Device::globalDevice, &bufferInfo, nullptr,
                       &buffer->vkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device::globalDevice, buffer->vkBuffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Device::globalInstance->findMemoryType(
        memRequirements.memoryTypeBits,
        (memoryUsage == MemoryUsageType::GPUOnly)
            ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        : (MemoryUsageType::CPUToGPU == memoryUsage)
            ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
               VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
    if (vkAllocateMemory(Device::globalDevice, &allocInfo, nullptr,
                         &buffer->vkBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(Device::globalDevice, buffer->vkBuffer,
                       buffer->vkBufferMemory, 0);
    void *mappedData;
    vkMapMemory(Device::globalDevice, buffer->vkBufferMemory, 0, size, 0,
                &mappedData);
    if (data) {
        memcpy(mappedData, data, size);
    }
    vkUnmapMemory(Device::globalDevice, buffer->vkBufferMemory);
#endif

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
    void *mappedData;
    vkMapMemory(Device::globalDevice, vkBufferMemory, offset, size, 0,
                &mappedData);
    memcpy(mappedData, data, size);
    vkUnmapMemory(Device::globalDevice, vkBufferMemory);
#endif
}

void Buffer::bind() const {
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
}

void Buffer::unbind() const {
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
    const std::vector<VertexAttributeBinding> &bindings) const {
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
#endif
}

} // namespace opal
