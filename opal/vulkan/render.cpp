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
#include <iostream>

namespace opal {

void CommandBuffer::record(uint32_t imageIndex) {
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        throw std::runtime_error("Cannot record command buffer: no render pass "
                                 "bound. Call bindPipeline() before draw().");
    }

    if (framebuffer == nullptr || framebuffer->vkFramebuffers.empty()) {
        throw std::runtime_error("Cannot record command buffer: invalid "
                                 "framebuffer or no Vulkan framebuffers.");
    }

    uint32_t fbIndex =
        (framebuffer->vkFramebuffers.size() == 1) ? 0 : imageIndex;
    if (fbIndex >= framebuffer->vkFramebuffers.size()) {
        throw std::runtime_error("Cannot record command buffer: image index "
                                 "out of range.");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->currentRenderPass->renderPass;
    renderPassInfo.framebuffer = framebuffer->vkFramebuffers[fbIndex];
    renderPassInfo.renderArea.offset = {.x = 0, .y = 0};

    if (framebuffer->vkFramebuffers.size() == 1 && framebuffer->width > 0) {
        renderPassInfo.renderArea.extent.width =
            static_cast<uint32_t>(framebuffer->width);
        renderPassInfo.renderArea.extent.height =
            static_cast<uint32_t>(framebuffer->height);
    } else {
        renderPassInfo.renderArea.extent = device->swapChainExtent;
    }

    std::vector<VkClearValue> clearValues;
    if (framebuffer->isDefaultFramebuffer) {
        VkClearValue colorClear{};
        colorClear.color = {{clearColorValue[0], clearColorValue[1],
                             clearColorValue[2], clearColorValue[3]}};
        VkClearValue brightClear{};
        brightClear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues.push_back(colorClear);
        clearValues.push_back(brightClear);
        // Add depth clear value if depth texture exists
        if (device->swapChainDepthTexture != nullptr) {
            VkClearValue depthClear{};
            depthClear.depthStencil = {.depth = 1.0f, .stencil = 0};
            clearValues.push_back(depthClear);
        }
    } else {
        for (const auto &attachment : framebuffer->attachments) {
            VkClearValue clear{};
            if (attachment.type == Attachment::Type::Color) {
                clear.color = {{clearColorValue[0], clearColorValue[1],
                                clearColorValue[2], clearColorValue[3]}};
            } else {
                clear.depthStencil = {.depth = clearDepthValue, .stencil = 0};
            }
            clearValues.push_back(clear);
        }
    }

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    std::cout << "[VULKAN DEBUG] vkCmdBeginRenderPass: imageIndex="
              << imageIndex << ", fbIndex=" << fbIndex
              << ", extent=" << renderPassInfo.renderArea.extent.width << "x"
              << renderPassInfo.renderArea.extent.height
              << ", clearValues=" << clearValues.size() << std::endl;

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    // Configure dynamic viewport/scissor
    VkViewport viewport{};
    if (renderPass->currentRenderPass &&
        renderPass->currentRenderPass->opalPipeline) {
        // Use the pipeline's viewport if it has been set (width != 0)
        // This allows for custom viewports (e.g. flipped Y for text)
        if (renderPass->currentRenderPass->opalPipeline->vkViewport.width !=
            0.0f) {
            viewport = renderPass->currentRenderPass->opalPipeline->vkViewport;
        } else {
            // Fallback to render area if pipeline viewport is not set
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width =
                static_cast<float>(renderPassInfo.renderArea.extent.width);
            viewport.height =
                static_cast<float>(renderPassInfo.renderArea.extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
        }
    } else {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width =
            static_cast<float>(renderPassInfo.renderArea.extent.width);
        viewport.height =
            static_cast<float>(renderPassInfo.renderArea.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = renderPassInfo.renderArea.extent;
    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
}

void CommandBuffer::createSyncObjects() {
    // Only create sync objects if they don't exist yet
    if (!imageAvailableSemaphores.empty()) {
        return; // Already created
    }

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device->logicalDevice, &fenceInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create synchronization objects!");
        }
    }
}

void CommandBuffer::beginCommandBufferIfNeeded() {
    if (commandBufferBegan) {
        return; // Already began this frame
    }

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    commandBufferBegan = true;
}

} // namespace opal

#endif