//
// command_buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The Command Buffer implementation for drawing commands
// Copyright (c) 2025 maxvdec
//

#include "atlas/tracer/data.h"
#include <array>
#include <cstdint>
#include <memory>
#include <opal/opal.h>
#include <iostream>
#include <string>

namespace opal {

std::shared_ptr<CommandBuffer> Device::acquireCommandBuffer() {
    auto commandBuffer = std::make_shared<CommandBuffer>();
    commandBuffer->device = this;

#ifdef VULKAN
    commandBuffer->commandBuffers.resize(CommandBuffer::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = Device::globalInstance->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = CommandBuffer::MAX_FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(this->logicalDevice, &allocInfo,
                                 commandBuffer->commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
#endif
    return commandBuffer;
}

void CommandBuffer::start() {
    boundPipeline = nullptr;
    boundDrawingState = nullptr;
    renderPass = nullptr;
    framebuffer = nullptr;
#ifdef VULKAN
    hasStarted = false;
    imageAcquired = false;
    commandBufferBegan = false;
    this->createSyncObjects();

    // Wait for the previous frame using this slot to complete
    vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame],
                    VK_TRUE, UINT64_MAX);

    // NOTE: We reset the fence in commit() right before vkQueueSubmit, not
    // here. This ensures we only reset when we're actually about to submit
    // work. If we reset here and then don't submit, the next start() would
    // hang.
#endif
}

void Device::submitCommandBuffer(
    [[maybe_unused]] std::shared_ptr<CommandBuffer> commandBuffer) {}

void CommandBuffer::beginPass(std::shared_ptr<RenderPass> newRenderPass) {
    if (newRenderPass == nullptr) {
        throw std::runtime_error(
            "Cannot begin a command buffer pass without a render pass");
    }
    if (newRenderPass->framebuffer == nullptr) {
        throw std::runtime_error(
            "Render pass must have a framebuffer before beginPass");
    }

    renderPass = std::move(newRenderPass);
    framebuffer = renderPass->framebuffer;

#ifdef OPENGL
    framebuffer->bind();
#elif defined(VULKAN)
    if (framebuffer->isDefaultFramebuffer && device != nullptr) {
        // Skip rendering if swapchain has zero extent (minimized window)
        if (device->swapChainExtent.width == 0 ||
            device->swapChainExtent.height == 0) {
            framebuffer = nullptr;
            renderPass = nullptr;
            return;
        }
        framebuffer->width = static_cast<int>(device->swapChainExtent.width);
        framebuffer->height = static_cast<int>(device->swapChainExtent.height);
    }
#endif
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

void CommandBuffer::endPass() {
#ifdef VULKAN
    if (hasStarted) {
        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        hasStarted = false;

        if (framebuffer != nullptr) {
            if (framebuffer->isDefaultFramebuffer) {
                // Default framebuffer uses swapchain images + bright + depth.
                // We set final layouts in the render pass to shader-read for
                // bright/depth; track that on the Texture objects so later
                // sampling doesn’t see stale DEPTH_ATTACHMENT layout.
                if (device != nullptr) {
                    if (device->swapChainDepthTexture != nullptr) {
                        device->swapChainDepthTexture->currentLayout =
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    if (!device->swapChainBrightTextures.empty() &&
                        imageAcquired &&
                        imageIndex < device->swapChainBrightTextures.size()) {
                        auto &brightTex =
                            device->swapChainBrightTextures[imageIndex];
                        if (brightTex != nullptr) {
                            brightTex->currentLayout =
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        }
                    }
                }
            } else {
                for (auto &attachment : framebuffer->attachments) {
                    if (attachment.texture != nullptr) {
                        attachment.texture->currentLayout =
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                }
            }
        }
    }
#endif
}

int CommandBuffer::getAndResetDrawCallCount() {
    int count = drawCallCount;
    drawCallCount = 0;
    return count;
}

void CommandBuffer::commit() {
#ifdef VULKAN
    // Skip commit if swapchain has zero extent (minimized window)
    if (device != nullptr && (device->swapChainExtent.width == 0 ||
                              device->swapChainExtent.height == 0)) {
        // Reset state for next frame - no work was submitted
        commandBufferBegan = false;
        return;
    }

    // If we haven't acquired an image yet and we're rendering to the default
    // framebuffer, we need to acquire one. But only if we have a valid render
    // pass set up.
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
            // No render pass means nothing was rendered - skip this frame
            // Don't acquire an image since we have no work to submit
            commandBufferBegan = false;
            return;
        }

        VkResult acquireResult = vkAcquireNextImageKHR(
            device->logicalDevice, device->swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
            &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR ||
            acquireResult == VK_SUBOPTIMAL_KHR) {
            // Swapchain needs recreation - skip this frame
            device->swapchainDirty = true;
            commandBufferBegan = false;
            return;
        }

        imageAcquired = true;

        beginCommandBufferIfNeeded();
        hasStarted = this->record(imageIndex);
    }

    // If we still don't have an image acquired, there's nothing to present
    if (!imageAcquired) {
        commandBufferBegan = false;
        return;
    }

    // If no command buffer was begun, we can't submit anything.
    // But we already acquired an image, so we need to submit SOMETHING to
    // consume the semaphore and signal the fence.
    if (!commandBufferBegan) {
        // Begin an empty command buffer just to maintain sync
        beginCommandBufferIfNeeded();
    }

    if (hasStarted) {
        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        hasStarted = false;
    }

    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the fence right before submit - this ensures we only reset when
    // we're actually about to submit work that will signal it
    vkResetFences(device->logicalDevice, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
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

    imageAcquired = false;
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
#endif
}

void CommandBuffer::bindPipeline(std::shared_ptr<Pipeline> pipeline) {
    pipeline->bind();
#ifdef VULKAN
#endif
    boundPipeline = pipeline;
#ifdef VULKAN

    std::shared_ptr<CoreRenderPass> coreRenderPass = nullptr;

    for (auto &corePipeline : RenderPass::cachedRenderPasses) {
        bool renderPassIsDefault =
            renderPass->framebuffer->isDefaultFramebuffer;
        bool cachedIsDefault =
            corePipeline->opalFramebuffer->isDefaultFramebuffer;

        bool framebufferMatch = false;
        if (renderPassIsDefault && cachedIsDefault) {
            framebufferMatch = true;
        } else if (!renderPassIsDefault && !cachedIsDefault) {
            framebufferMatch = (corePipeline->opalFramebuffer->attachments ==
                                renderPass->framebuffer->attachments);
        }

        // Only reuse cached CoreRenderPass when using the exact same Pipeline
        // instance. Reusing across "equivalent" pipelines is unsafe because
        // descriptor set layouts / push constant ranges can differ, and Vulkan
        // requires layout compatibility between vkCmdBindPipeline and
        // vkCmdBindDescriptorSets/vkCmdPushConstants.
        if (framebufferMatch &&
            corePipeline->opalPipeline.get() == pipeline.get()) {
            renderPass->currentRenderPass = corePipeline;
            coreRenderPass = corePipeline;
            break;
        }
    }

    if (coreRenderPass == nullptr) {
        if (hasStarted && renderPass->currentRenderPass != nullptr &&
            renderPass->currentRenderPass->renderPass != VK_NULL_HANDLE) {
            coreRenderPass = CoreRenderPass::createWithExistingRenderPass(
                pipeline, renderPass->framebuffer,
                renderPass->currentRenderPass->renderPass);
        } else {
            coreRenderPass =
                CoreRenderPass::create(pipeline, renderPass->framebuffer);
        }
        renderPass->currentRenderPass = coreRenderPass;
    }

    if (framebuffer->isDefaultFramebuffer) {
        // Guard against creating framebuffers with zero extent
        if (device->swapChainExtent.width == 0 ||
            device->swapChainExtent.height == 0) {
            return;
        }
        if (device->swapchainDirty || framebuffer->vkFramebuffers.size() == 0) {
            framebuffer->vkFramebuffers.resize(
                device->swapChainImages.imageViews.size());
            if (device->swapChainBrightTextures.size() !=
                device->swapChainImages.imageViews.size()) {
                device->createSwapChainBrightTextures();
            }
            for (size_t i = 0; i < device->swapChainImages.imageViews.size();
                 i++) {
                auto &brightTextures = device->swapChainBrightTextures;
                if (brightTextures.size() <= i ||
                    brightTextures[i] == nullptr ||
                    brightTextures[i]->vkImageView == VK_NULL_HANDLE) {
                    throw std::runtime_error(
                        "Swapchain bright attachments are not initialized");
                }

                std::vector<VkImageView> attachments = {
                    device->swapChainImages.imageViews[i],
                    brightTextures[i]->vkImageView};

                if (device->swapChainDepthTexture != nullptr &&
                    device->swapChainDepthTexture->vkImageView !=
                        VK_NULL_HANDLE) {
                    attachments.push_back(
                        device->swapChainDepthTexture->vkImageView);
                }

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType =
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = coreRenderPass->renderPass;
                framebufferInfo.attachmentCount =
                    static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = device->swapChainExtent.width;
                framebufferInfo.height = device->swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(
                        device->logicalDevice, &framebufferInfo, nullptr,
                        &framebuffer->vkFramebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }

            device->swapchainDirty = false;
        }
    } else {
        if (framebuffer->vkFramebuffers.empty()) {
            framebuffer->createVulkanFramebuffers(coreRenderPass);
        }
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
                         [[maybe_unused]] uint firstInstance, int objectId)
    -> void {
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
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        hasStarted = this->record(imageIndex);
    }
    if (!hasStarted) {
        return; // Render pass not started, skip draw
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);

    // Use boundPipeline for all pipeline operations - it's the one the user set
    // uniforms on
    Pipeline *activePipeline = boundPipeline.get();
    if (activePipeline == nullptr &&
        renderPass->currentRenderPass->opalPipeline != nullptr) {
        activePipeline = renderPass->currentRenderPass->opalPipeline.get();
    }
    if (activePipeline != nullptr) {
        activePipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (activePipeline != nullptr) {
        VkViewport viewport = activePipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        activePipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, instanceCount,
              firstVertex, firstInstance);
#endif

    DrawCallInfo info;
    info.callerObject = std::to_string(objectId);
    info.frameNumber = (int)device->frameCount;
    info.type = DrawCallType::Draw;
    info.send();

    drawCallCount++;
}

void CommandBuffer::drawIndexed(uint indexCount, uint instanceCount,
                                uint firstIndex,
                                [[maybe_unused]] int vertexOffset,
                                [[maybe_unused]] uint firstInstance,
                                int objectId) {
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
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        hasStarted = this->record(imageIndex);
    }
    if (!hasStarted) {
        return; // Render pass not started, skip draw
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);

    // Use boundPipeline for all pipeline operations - it's the one the user set
    // uniforms on
    Pipeline *activePipeline = boundPipeline.get();
    if (activePipeline == nullptr &&
        renderPass->currentRenderPass->opalPipeline != nullptr) {
        activePipeline = renderPass->currentRenderPass->opalPipeline.get();
    }
    if (activePipeline != nullptr) {
        activePipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    if (boundDrawingState != nullptr) {
        bindVertexBuffersIfNeeded();
        if (boundDrawingState->indexBuffer != nullptr) {
            vkCmdBindIndexBuffer(commandBuffers[currentFrame],
                                 boundDrawingState->indexBuffer->vkBuffer, 0,
                                 VK_INDEX_TYPE_UINT32);
        }
    }
    if (activePipeline != nullptr) {
        VkViewport viewport = activePipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        activePipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, instanceCount,
                     firstIndex, vertexOffset, firstInstance);
#endif

    DrawCallInfo info;
    info.callerObject = std::to_string(objectId);
    info.frameNumber = (int)device->frameCount;
    info.type = DrawCallType::Indexed;
    info.send();

    drawCallCount++;
}

void CommandBuffer::drawPatches(uint vertexCount, uint firstVertex,
                                int objectId) {
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
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        VkResult result = vkAcquireNextImageKHR(
            device->logicalDevice, device->swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
            &imageIndex);
        imageAcquired = true;
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            device->remakeSwapChain(device->context);
            if (framebuffer->isDefaultFramebuffer) {
                // Centralized recreation: ensures non-zero swapchain extent,
                // destroys old VkFramebuffers, and updates width/height.
                framebuffer->createVulkanFramebuffers(
                    renderPass->currentRenderPass);
            } else {
                device->swapchainDirty = true;
            }
            return;
        }
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        hasStarted = this->record(imageIndex);
    }
    if (!hasStarted) {
        return; // Render pass not started, skip draw
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);

    // Use boundPipeline for all pipeline operations - it's the one the user set
    // uniforms on
    Pipeline *activePipeline = boundPipeline.get();
    if (activePipeline == nullptr &&
        renderPass->currentRenderPass->opalPipeline != nullptr) {
        activePipeline = renderPass->currentRenderPass->opalPipeline.get();
    }
    if (activePipeline != nullptr) {
        activePipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (activePipeline != nullptr) {
        VkViewport viewport = activePipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        activePipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, 1, firstVertex, 0);
#endif

    DrawCallInfo info;
    info.callerObject = std::to_string(objectId);
    info.frameNumber = device->frameCount;
    info.type = DrawCallType::Patch;
    info.send();

    drawCallCount++;
}

#ifdef VULKAN
void CommandBuffer::bindVertexBuffersIfNeeded() {
    if (boundDrawingState == nullptr ||
        boundDrawingState->vertexBuffer == nullptr) {
        return;
    }

    Pipeline *activePipeline = nullptr;
    if (renderPass != nullptr && renderPass->currentRenderPass != nullptr &&
        renderPass->currentRenderPass->opalPipeline != nullptr) {
        activePipeline = renderPass->currentRenderPass->opalPipeline.get();
    }

    std::array<VkBuffer, 2> buffers = {
        boundDrawingState->vertexBuffer->vkBuffer, VK_NULL_HANDLE};
    std::array<VkDeviceSize, 2> offsets = {0, 0};
    uint32_t bindingCount = 1;

    bool needsInstanceBinding =
        activePipeline != nullptr && activePipeline->hasInstanceAttributes;

    VkBuffer instanceBufferHandle = VK_NULL_HANDLE;
    if (boundDrawingState->instanceBuffer != nullptr) {
        instanceBufferHandle = boundDrawingState->instanceBuffer->vkBuffer;
    }

    if (needsInstanceBinding && instanceBufferHandle == VK_NULL_HANDLE &&
        device != nullptr) {
        auto fallback = device->getDefaultInstanceBuffer();
        if (fallback != nullptr) {
            instanceBufferHandle = fallback->vkBuffer;
        }
    }

    if (needsInstanceBinding || instanceBufferHandle != VK_NULL_HANDLE) {
        buffers[1] = instanceBufferHandle;
        bindingCount = 2;
    }

    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, bindingCount,
                           buffers.data(), offsets.data());
}
#endif

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
