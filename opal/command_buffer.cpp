//
// command_buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The Command Buffer implementation for drawing commands
// Copyright (c) 2025 maxvdec
//

#include <cstdint>
#include <memory>
#include <opal/opal.h>

namespace opal {

std::shared_ptr<CommandBuffer> Device::acquireCommandBuffer() {
    auto commandBuffer = std::make_shared<CommandBuffer>();
    commandBuffer->device = this;

#ifdef VULKAN
    if (this->commandPool == VK_NULL_HANDLE) {
        QueueFamilyIndices indices =
            findQueueFamilies(physicalDevice, context->surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr,
                                &this->commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(this->logicalDevice, &allocInfo,
                                 &commandBuffer->commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
#endif
    return commandBuffer;
}

void CommandBuffer::start() {
    // Reset state for a new command recording session
    boundPipeline = nullptr;
    boundDrawingState = nullptr;
#ifdef VULKAN
    hasStarted = false;
    this->createSyncObjects();
    vkWaitForFences(device->logicalDevice, 1, &inFlightFence, VK_TRUE,
                    UINT64_MAX);
    vkResetFences(device->logicalDevice, 1, &inFlightFence);
#endif
}

void Device::submitCommandBuffer(
    [[maybe_unused]] std::shared_ptr<CommandBuffer> commandBuffer) {}

void CommandBuffer::beginPass(
    [[maybe_unused]] std::shared_ptr<RenderPass> renderPass) {
    if (renderPass != nullptr) {
        renderPass->framebuffer->bind();
    }
    this->framebuffer = renderPass->framebuffer;
    this->renderPass = renderPass;
}

void CommandBuffer::beginSampled(
    [[maybe_unused]] std::shared_ptr<Framebuffer> readFramebuffer,
    [[maybe_unused]] std::shared_ptr<Framebuffer> writeFramebuffer) {
    if (writeFramebuffer != nullptr) {
        writeFramebuffer->bindForDraw();
    }
    if (readFramebuffer != nullptr) {
        readFramebuffer->bindForRead();
    }
}

void CommandBuffer::endPass() {}
void CommandBuffer::commit() {
#ifdef VULKAN
    if (hasStarted) {
        vkCmdEndRenderPass(this->commandBuffer);
        hasStarted = false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFence) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {device->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(device->presentQueue, &presentInfo);
#endif
}

void CommandBuffer::bindPipeline(std::shared_ptr<Pipeline> pipeline) {
    pipeline->bind();
#ifdef VULKAN
    if (boundPipeline != nullptr) {
        vkCmdEndRenderPass(this->commandBuffer);
        hasStarted = false;
    }
#endif
    boundPipeline = pipeline;
#ifdef VULKAN

    for (auto &corePipeline : RenderPass::cachedRenderPasses) {
        if (corePipeline->opalFramebuffer->attachments ==
            renderPass->framebuffer->attachments) {
            if (*corePipeline->opalPipeline == pipeline) {
                renderPass->currentRenderPass = corePipeline;
                return;
            }
        }
    }
    auto coreRenderPass =
        CoreRenderPass::create(pipeline, renderPass->framebuffer);
    renderPass->currentRenderPass = coreRenderPass;

    if (framebuffer->framebufferID == 0 &&
        (device->swapchainDirty || framebuffer->vkFramebuffers.size() == 0)) {
        framebuffer->vkFramebuffers.resize(
            device->swapChainImages.imageViews.size());
        for (size_t i = 0; i < device->swapChainImages.imageViews.size(); i++) {
            VkImageView attachments[] = {device->swapChainImages.imageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = coreRenderPass->renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = device->swapChainExtent.width;
            framebufferInfo.height = device->swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device->logicalDevice, &framebufferInfo,
                                    nullptr, &framebuffer->vkFramebuffers[i]) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

        device->swapchainDirty = false;
    }
#endif
}

void CommandBuffer::unbindPipeline() { boundPipeline = nullptr; }

void CommandBuffer::bindDrawingState(
    std::shared_ptr<DrawingState> drawingState) {
    boundDrawingState = drawingState;
}

void CommandBuffer::unbindDrawingState() { boundDrawingState = nullptr; }

auto CommandBuffer::draw(uint vertexCount, uint instanceCount, uint firstVertex,
                         [[maybe_unused]] uint firstInstance) -> void {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    glDrawArraysInstanced(GL_TRIANGLES, firstVertex, vertexCount,
                          instanceCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#elif defined(VULKAN)
    if (!hasStarted) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX, imageAvailableSemaphore,
                              VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(this->commandBuffer, 0);
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    vkCmdDraw(this->commandBuffer, vertexCount, instanceCount, firstVertex,
              firstInstance);
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
#elif defined(VULKAN)
    if (!hasStarted) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX, imageAvailableSemaphore,
                              VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(this->commandBuffer, 0);
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    vkCmdDrawIndexed(this->commandBuffer, indexCount, instanceCount, firstIndex,
                     vertexOffset, firstInstance);
#endif
}

void CommandBuffer::drawPatches(uint vertexCount, uint firstVertex) {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    if (boundPipeline != nullptr) {
        glPatchParameteri(GL_PATCH_VERTICES, boundPipeline->getPatchVertices());
    }
    glDrawArrays(GL_PATCHES, firstVertex, vertexCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#elif defined(VULKAN)
    if (!hasStarted) {
        VkResult result = vkAcquireNextImageKHR(
            device->logicalDevice, device->swapChain, UINT64_MAX,
            imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            device->remakeSwapChain(device->context);
            if (framebuffer->framebufferID == 0) {
                framebuffer->vkFramebuffers.resize(
                    device->swapChainImages.imageViews.size());
                for (size_t i = 0;
                     i < device->swapChainImages.imageViews.size(); i++) {
                    VkImageView attachments[] = {
                        device->swapChainImages.imageViews[i]};

                    VkFramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.sType =
                        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferInfo.renderPass =
                        renderPass->currentRenderPass->renderPass;
                    framebufferInfo.attachmentCount = 1;
                    framebufferInfo.pAttachments = attachments;
                    framebufferInfo.width = device->swapChainExtent.width;
                    framebufferInfo.height = device->swapChainExtent.height;
                    framebufferInfo.layers = 1;

                    if (vkCreateFramebuffer(
                            device->logicalDevice, &framebufferInfo, nullptr,
                            &framebuffer->vkFramebuffers[i]) != VK_SUCCESS) {
                        throw std::runtime_error(
                            "failed to create framebuffer!");
                    }
                }
            } else {
                device->swapchainDirty = true;
            }
        }
        vkResetCommandBuffer(this->commandBuffer, 0);
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    vkCmdDraw(this->commandBuffer, vertexCount, 1, firstVertex, 0);
#endif
}

void CommandBuffer::clearColor(float r, float g, float b, float a) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;
}

void CommandBuffer::clearDepth(float depth) {
#ifdef OPENGL
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
#endif
    this->clearDepthValue = depth;
}

void CommandBuffer::clear(float r, float g, float b, float a, float depth) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;
    this->clearDepthValue = depth;
}

} // namespace opal
