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

namespace opal {

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
    if (indexBuffer) {
        indexBuffer->unbind();
    }
    if (vertexBuffer) {
        vertexBuffer->unbind();
    }
    glBindVertexArray(0);
#endif
}

} // namespace opal
