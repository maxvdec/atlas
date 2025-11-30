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

} // namespace opal

#endif