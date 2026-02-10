//
// framebuffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Vulkan framebuffer implementation
// Copyright (c) 2025 maxvdec
//

#ifdef VULKAN
#include <array>
#include <vector>
#include <opal/opal.h>
#include <vulkan/vulkan.hpp>

namespace opal {

void Framebuffer::createVulkanFramebuffers(
    std::shared_ptr<CoreRenderPass> renderPass) {
    if (!Device::globalInstance) {
        throw std::runtime_error(
            "Device not initialized for Vulkan framebuffer creation");
    }

    for (auto fb : vkFramebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(Device::globalDevice, fb, nullptr);
        }
    }
    vkFramebuffers.clear();

    if (isDefaultFramebuffer) {
        Device *device = Device::globalInstance;

        if (device->swapChainExtent.width == 0 ||
            device->swapChainExtent.height == 0) {
            throw std::runtime_error(
                "Swapchain extent is 0x0; cannot create Vulkan framebuffers. "
                "This usually happens when the window framebuffer size is 0 "
                "during startup or while minimized.");
        }

        vkFramebuffers.resize(device->swapChainImages.imageViews.size());
        if (device->swapChainBrightTextures.size() !=
            device->swapChainImages.imageViews.size()) {
            device->createSwapChainBrightTextures();
        }

        for (size_t i = 0; i < device->swapChainImages.imageViews.size(); i++) {
            auto &brightTextures = device->swapChainBrightTextures;
            if (brightTextures.size() <= i || brightTextures[i] == nullptr ||
                brightTextures[i]->vkImageView == VK_NULL_HANDLE) {
                throw std::runtime_error(
                    "Swapchain bright attachments are not initialized");
            }

            std::vector<VkImageView> attachmentViews = {
                device->swapChainImages.imageViews[i],
                brightTextures[i]->vkImageView};

            // Add depth attachment if available
            if (device->swapChainDepthTexture &&
                device->swapChainDepthTexture->vkImageView != VK_NULL_HANDLE) {
                attachmentViews.push_back(
                    device->swapChainDepthTexture->vkImageView);
            }

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass->renderPass;
            framebufferInfo.attachmentCount =
                static_cast<uint32_t>(attachmentViews.size());
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.width = device->swapChainExtent.width;
            framebufferInfo.height = device->swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(Device::globalDevice, &framebufferInfo,
                                    nullptr,
                                    &vkFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error(
                    "Failed to create Vulkan framebuffer!");
            }
        }

        this->width = static_cast<int>(device->swapChainExtent.width);
        this->height = static_cast<int>(device->swapChainExtent.height);
    } else {
        // Skip creating framebuffers with zero dimensions - they'll be created
        // later
        if (this->width <= 0 || this->height <= 0) {
            return;
        }

        std::vector<VkImageView> attachmentViews;
        for (const auto &attachment : attachments) {
            attachmentViews.push_back(attachment.texture->vkImageView);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass->renderPass;
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachmentViews.size());
        framebufferInfo.pAttachments = attachmentViews.data();
        framebufferInfo.width = static_cast<uint32_t>(this->width);
        framebufferInfo.height = static_cast<uint32_t>(this->height);
        framebufferInfo.layers = 1;

        VkFramebuffer fb;
        if (vkCreateFramebuffer(Device::globalDevice, &framebufferInfo, nullptr,
                                &fb) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan framebuffer!");
        }
        vkFramebuffers.push_back(fb);
    }
}

void Framebuffer::transitionImageLayout(VkImage image, VkFormat format,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout,
                                        uint32_t layerCount) {
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

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    // Determine aspect mask based on format
    if (format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D16_UNORM) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        // Generic transition
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

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
}

void RenderPass::applyRenderPass() {
    for (const auto &cached : cachedRenderPasses) {
        if (cached->opalFramebuffer == this->framebuffer) {
            this->currentRenderPass = cached;
            return;
        }
    }

    auto renderPass = std::make_shared<CoreRenderPass>();
    renderPass->opalFramebuffer = this->framebuffer;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentReference depthAttachmentRef{};
    bool hasDepthAttachment = false;

    if (this->framebuffer->isDefaultFramebuffer) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = Device::globalInstance->swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(colorRef);

        VkAttachmentDescription brightAttachment{};
        VkFormat brightFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        if (Device::globalInstance != nullptr &&
            !Device::globalInstance->swapChainBrightTextures.empty() &&
            Device::globalInstance->swapChainBrightTextures[0] != nullptr) {
            brightFormat = opalTextureFormatToVulkanFormat(
                Device::globalInstance->swapChainBrightTextures[0]->format);
        }
        brightAttachment.format = brightFormat;
        brightAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        brightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        brightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        brightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkImageLayout brightInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (Device::globalInstance != nullptr &&
            !Device::globalInstance->swapChainBrightTextures.empty() &&
            Device::globalInstance->swapChainBrightTextures[0] != nullptr &&
            Device::globalInstance->swapChainBrightTextures[0]->currentLayout !=
                VK_IMAGE_LAYOUT_UNDEFINED) {
            brightInitialLayout =
                Device::globalInstance->swapChainBrightTextures[0]->currentLayout;
        }
        brightAttachment.initialLayout = brightInitialLayout;
        brightAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(brightAttachment);

        VkAttachmentReference brightRef{};
        brightRef.attachment = 1;
        brightRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(brightRef);

        // Add depth attachment for default framebuffer if depth texture exists
        if (Device::globalInstance->swapChainDepthTexture != nullptr) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = VK_FORMAT_D32_SFLOAT;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            VkImageLayout depthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (Device::globalInstance != nullptr &&
                Device::globalInstance->swapChainDepthTexture != nullptr &&
                Device::globalInstance->swapChainDepthTexture->currentLayout !=
                    VK_IMAGE_LAYOUT_UNDEFINED) {
                depthInitialLayout =
                    Device::globalInstance->swapChainDepthTexture->currentLayout;
            }
            depthAttachment.initialLayout = depthInitialLayout;
            depthAttachment.finalLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachments.push_back(depthAttachment);

            depthAttachmentRef.attachment = 2; // Index 2 after color and bright
            depthAttachmentRef.layout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepthAttachment = true;
        }
    } else {
        for (uint32_t i = 0; i < this->framebuffer->attachments.size(); ++i) {
            const auto &attachment = this->framebuffer->attachments[i];

            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format =
                opalTextureFormatToVulkanFormat(attachment.texture->format);
            attachmentDesc.samples = attachment.texture->samples > 1
                                         ? VK_SAMPLE_COUNT_4_BIT
                                         : VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (attachment.texture != nullptr &&
                attachment.texture->currentLayout !=
                    VK_IMAGE_LAYOUT_UNDEFINED) {
                initialLayout = attachment.texture->currentLayout;
            }
            attachmentDesc.initialLayout = initialLayout;

            if (attachment.type == Attachment::Type::Color) {
                attachmentDesc.finalLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkAttachmentReference colorRef{};
                colorRef.attachment = i;
                colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs.push_back(colorRef);
            } else if (attachment.type == Attachment::Type::Depth ||
                       attachment.type == Attachment::Type::DepthStencil) {
                // Depth attachments (shadow maps, gbuffer depth, etc.) are
                // commonly sampled in later passes. Use SHADER_READ_ONLY as
                // the final layout so they can be sampled without an extra
                // transition.
                attachmentDesc.finalLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                depthAttachmentRef.attachment = i;
                depthAttachmentRef.layout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                hasDepthAttachment = true;
            }

            attachments.push_back(attachmentDesc);
        }
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount =
        static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    if (hasDepthAttachment) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    VkSubpassDependency dependencies[2] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(Device::globalDevice, &renderPassInfo, nullptr,
                           &renderPass->renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    this->framebuffer->createVulkanFramebuffers(renderPass);

    cachedRenderPasses.push_back(renderPass);
    this->currentRenderPass = renderPass;
}

} // namespace opal

#endif
