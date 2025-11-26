//
// command_buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The Command Buffer implementation for drawing commands
// Copyright (c) 2025 maxvdec
//

#include <memory>
#include <opal/opal.h>

namespace opal {

std::shared_ptr<CommandBuffer> Device::acquireCommandBuffer() {
    auto commandBuffer = std::make_shared<CommandBuffer>();
    return commandBuffer;
}

void Device::submitCommandBuffer(
    [[maybe_unused]] std::shared_ptr<CommandBuffer> commandBuffer) {}

void CommandBuffer::begin() {}
void CommandBuffer::end() {}

void CommandBuffer::bindPipeline(std::shared_ptr<Pipeline> pipeline) {
    pipeline->bind();
    boundPipeline = pipeline;
}

void CommandBuffer::unbindPipeline() { boundPipeline = nullptr; }

void CommandBuffer::bindDrawingState(
    std::shared_ptr<DrawingState> drawingState) {
    boundDrawingState = drawingState;
}

void CommandBuffer::unbindDrawingState() { boundDrawingState = nullptr; }

void CommandBuffer::draw(uint vertexCount, uint instanceCount, uint firstVertex,
                         [[maybe_unused]] uint firstInstance) {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    glDrawArraysInstanced(GL_TRIANGLES, firstVertex, vertexCount,
                          instanceCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#endif
}

void CommandBuffer::drawIndexed(uint indexCount, uint instanceCount,
                                uint firstIndex,
                                [[maybe_unused]] int vertexOffset,
                                [[maybe_unused]] uint firstInstance) {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT,
                            (void *)(uintptr_t)(firstIndex * sizeof(uint)),
                            instanceCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#endif
}

void CommandBuffer::clearColor(float r, float g, float b, float a) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
}

void CommandBuffer::clearDepth(float depth) {
#ifdef OPENGL
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
#endif
}

} // namespace opal
