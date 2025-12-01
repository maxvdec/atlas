//
// render.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Render functions and command buffer
// Copyright (c) 2025 Max Van den Eynde
//

#ifdef VULKAN
#include <opal/opal.h>
#include <vulkan/vulkan.hpp>

namespace opal {

void CommandBuffer::record(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    if (vkBeginCommandBuffer(this->commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        throw std::runtime_error("Cannot record command buffer: no render pass "
                                 "bound. Call bindPipeline() before draw().");
    }

    if (framebuffer == nullptr ||
        imageIndex >= framebuffer->vkFramebuffers.size()) {
        throw std::runtime_error("Cannot record command buffer: invalid "
                                 "framebuffer or image index.");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->currentRenderPass->renderPass;
    renderPassInfo.framebuffer = framebuffer->vkFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {.x = 0, .y = 0};
    renderPassInfo.renderArea.extent = device->swapChainExtent;

    VkClearValue clear{};
    clear.color = {{clearColorValue[0], clearColorValue[1], clearColorValue[2],
                    clearColorValue[3]}};
    clear.depthStencil = {.depth = clearDepthValue, .stencil = 0};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear;

    vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device->logicalDevice, &fenceInfo, nullptr,
                      &inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization objects!");
    }
}

} // namespace opal

#endif