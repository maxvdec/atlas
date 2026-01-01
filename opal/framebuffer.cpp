//
// framebuffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Framebuffer abstractions for Opal
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <memory>
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

std::shared_ptr<RenderPass> RenderPass::create() {
    return std::make_shared<RenderPass>();
}

void RenderPass::setFramebuffer(std::shared_ptr<Framebuffer> framebuffer) {
    this->framebuffer = framebuffer;
}

std::shared_ptr<Framebuffer> Framebuffer::create(int width, int height) {
    auto framebuffer = std::make_shared<Framebuffer>();
    framebuffer->width = width;
    framebuffer->height = height;

#ifdef OPENGL
    glGenFramebuffers(1, &framebuffer->framebufferID);
#elif defined(VULKAN)
#endif

    return framebuffer;
}

std::shared_ptr<Framebuffer> Framebuffer::create() {
    auto framebuffer = std::make_shared<Framebuffer>();
    framebuffer->width = 0;
    framebuffer->height = 0;

#ifdef OPENGL
    glGenFramebuffers(1, &framebuffer->framebufferID);
#elif defined(VULKAN)
#endif

    return framebuffer;
}

void Framebuffer::attachTexture(std::shared_ptr<Texture> texture,
                                int attachmentIndex) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum attachmentType =
        GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachmentIndex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, texture->glType,
                           texture->textureID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(VULKAN)
    if (attachmentIndex < 0) {
        return;
    }
    if (static_cast<size_t>(attachmentIndex) >= attachments.size()) {
        attachments.resize(static_cast<size_t>(attachmentIndex) + 1);
    }
    attachments[static_cast<size_t>(attachmentIndex)].type =
        Attachment::Type::Color;
    attachments[static_cast<size_t>(attachmentIndex)].texture = texture;

    // Attachment image views changed; force VkFramebuffer recreation.
    vkFramebuffers.clear();
#endif
}

void Framebuffer::addAttachment(const Attachment &attachment) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum attachmentType;
    switch (attachment.type) {
    case Attachment::Type::Color:
        attachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    case Attachment::Type::Depth:
        attachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Stencil:
        attachmentType = GL_STENCIL_ATTACHMENT;
        break;
    case Attachment::Type::DepthStencil:
        attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
        break;
    default:
        atlas_error("Unknown attachment type");
        throw std::runtime_error("Unknown attachment type");
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType,
                           attachment.texture->glType,
                           attachment.texture->textureID, 0);
    attachments.push_back(attachment);
#elif defined(VULKAN)
    attachments.push_back(attachment);
#endif
}

void Framebuffer::attachCubemap(std::shared_ptr<Texture> texture,
                                Attachment::Type attachmentType) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum glAttachmentType;
    switch (attachmentType) {
    case Attachment::Type::Depth:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Color:
        glAttachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    default:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    }
    // Use glFramebufferTexture for cubemaps (attaches all 6 faces)
    glFramebufferTexture(GL_FRAMEBUFFER, glAttachmentType, texture->textureID,
                         0);

    Attachment att;
    att.type = attachmentType;
    att.texture = texture;
    attachments.push_back(att);
#elif defined(VULKAN)
    Attachment att;
    att.type = attachmentType;
    att.texture = texture;
    attachments.push_back(att);
#endif
}

void Framebuffer::attachCubemapFace(std::shared_ptr<Texture> texture, int face,
                                    Attachment::Type attachmentType) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum glAttachmentType;
    switch (attachmentType) {
    case Attachment::Type::Depth:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Color:
        glAttachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    default:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    }
    // Attach a specific cubemap face (0-5: +X, -X, +Y, -Y, +Z, -Z)
    glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                           texture->textureID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(VULKAN)
    (void)face;
    Attachment att;
    att.type = attachmentType;
    att.texture = texture;
    attachments.push_back(att);
#endif
}

void Framebuffer::disableColorBuffer() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    colorBufferDisabled = true;
#elif defined(VULKAN)
    colorBufferDisabled = true;
#endif
}

void Framebuffer::setViewport() {
#ifdef OPENGL
    glViewport(0, 0, width, height);
#elif defined(VULKAN)
#endif
}

void Framebuffer::setViewport(int x, int y, int viewWidth, int viewHeight) {
#ifdef OPENGL
    glViewport(x, y, viewWidth, viewHeight);
#elif defined(VULKAN)
    (void)x;
    (void)y;
    (void)viewWidth;
    (void)viewHeight;
#endif
}

bool Framebuffer::getStatus() const {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
#elif defined(VULKAN)
    (void)this;
    return true;
#else
    return false;
#endif
}

void Framebuffer::bind() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    if (framebufferID == 0) {
        glDrawBuffer(GL_BACK);
    } else if (colorBufferDisabled) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else if (attachments.size() > 0) {
        std::vector<GLenum> drawBuffers;
        for (size_t i = 0; i < attachments.size(); ++i) {
            if (attachments[i].type == Attachment::Type::Color) {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 +
                                      static_cast<GLenum>(drawBuffers.size()));
            }
        }
        if (drawBuffers.empty()) {
            glDrawBuffer(GL_NONE);
        } else {
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()),
                          drawBuffers.data());
        }
    } else {
        glDrawBuffer(GL_NONE);
    }
#elif defined(VULKAN)
#endif
}

void Framebuffer::unbind() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(VULKAN)
#endif
}

void Framebuffer::bindForRead() {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferID);
#elif defined(VULKAN)
#endif
}

void Framebuffer::bindForDraw() {
#ifdef OPENGL
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferID);
#elif defined(VULKAN)
#endif
}

void Framebuffer::setDrawBuffers(int attachmentCount) {
#ifdef OPENGL
    if (attachmentCount <= 0) {
        glDrawBuffer(GL_NONE);
        return;
    }
    std::vector<GLenum> drawBuffers;
    drawBuffers.reserve(attachmentCount);
    for (int i = 0; i < attachmentCount; ++i) {
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glDrawBuffers(attachmentCount, drawBuffers.data());
#elif defined(VULKAN)
    (void)attachmentCount;
#endif
}

std::shared_ptr<ResolveAction>
ResolveAction::create(std::shared_ptr<Framebuffer> source,
                      std::shared_ptr<Framebuffer> destination) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = -1;
    action->resolveColor = true;
    action->resolveDepth = true;
    return action;
}

std::shared_ptr<ResolveAction> ResolveAction::createForColorAttachment(
    std::shared_ptr<Framebuffer> source,
    std::shared_ptr<Framebuffer> destination, int colorAttachmentIndex) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = colorAttachmentIndex;
    action->resolveColor = true;
    action->resolveDepth = false;
    return action;
}

std::shared_ptr<ResolveAction>
ResolveAction::createForDepth(std::shared_ptr<Framebuffer> source,
                              std::shared_ptr<Framebuffer> destination) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = -1;
    action->resolveColor = false;
    action->resolveDepth = true;
    return action;
}

void CommandBuffer::performResolve(std::shared_ptr<ResolveAction> action) {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, action->source->framebufferID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, action->destination->framebufferID);

    GLbitfield mask = 0;

    if (action->resolveColor) {
        if (action->colorAttachmentIndex >= 0) {
            // Resolve specific color attachment
            glReadBuffer(GL_COLOR_ATTACHMENT0 + action->colorAttachmentIndex);
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + action->colorAttachmentIndex);
            mask |= GL_COLOR_BUFFER_BIT;
        } else {
            // Resolve all color attachments
            mask |= GL_COLOR_BUFFER_BIT;
        }
    }

    if (action->resolveDepth) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if (mask != 0) {
        glBlitFramebuffer(0, 0, action->source->width, action->source->height,
                          0, 0, action->destination->width,
                          action->destination->height, mask, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(VULKAN)
    if (action == nullptr || action->source == nullptr ||
        action->destination == nullptr) {
        return;
    }

    if (Device::globalInstance == nullptr ||
        Device::globalInstance->commandPool == VK_NULL_HANDLE ||
        Device::globalInstance->graphicsQueue == VK_NULL_HANDLE) {
        return;
    }

    if (Device::globalDevice == VK_NULL_HANDLE) {
        return;
    }

    // Resolve/blit must be recorded on the same command buffer that will be
    // submitted for this frame. Using one-time command buffers + QueueWaitIdle
    // breaks Vulkan layout tracking and causes the exact VUID we're seeing.
    //
    // Ensure we are not inside a render pass.
    this->endPass();
    beginCommandBufferIfNeeded();

    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    auto gatherColorAttachments = [](std::shared_ptr<Framebuffer> fb,
                                     int preferredIndex) {
        std::vector<std::shared_ptr<Texture>> attachments;
        if (fb == nullptr) {
            return attachments;
        }

        int currentColorIndex = 0;
        for (const auto &attachment : fb->attachments) {
            if (attachment.type != Attachment::Type::Color) {
                continue;
            }
            if (preferredIndex >= 0) {
                if (currentColorIndex == preferredIndex) {
                    attachments.push_back(attachment.texture);
                    break;
                }
            } else {
                attachments.push_back(attachment.texture);
            }
            currentColorIndex++;
        }
        return attachments;
    };

    auto getDepthAttachment = [](std::shared_ptr<Framebuffer> fb) {
        if (fb == nullptr) {
            return std::shared_ptr<Texture>{nullptr};
        }
        for (const auto &attachment : fb->attachments) {
            if (attachment.type == Attachment::Type::Depth ||
                attachment.type == Attachment::Type::DepthStencil) {
                return attachment.texture;
            }
        }
        return std::shared_ptr<Texture>{nullptr};
    };

    auto aspectMaskForFormat = [](TextureFormat format,
                                  bool isDepth) -> VkImageAspectFlags {
        if (!isDepth) {
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
        switch (format) {
        case TextureFormat::Depth24Stencil8:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        case TextureFormat::DepthComponent24:
        case TextureFormat::Depth32F:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        default:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    };

    auto cmdTransitionImageLayout = [&](std::shared_ptr<Texture> texture,
                                        VkImageLayout newLayout,
                                        VkImageAspectFlags aspectMask,
                                        uint32_t layerCount) {
        if (!texture || texture->vkImage == VK_NULL_HANDLE) {
            return;
        }

        VkImageLayout oldLayout = texture->currentLayout;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            // If we truly don't know, treat UNDEFINED as the source.
            oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        if (oldLayout == newLayout) {
            return;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture->vkImage;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        // Common transitions used by resolve/blit.
        if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                   newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);

        texture->currentLayout = newLayout;
    };

    auto blitOrResolve = [&](std::shared_ptr<Texture> src,
                             std::shared_ptr<Texture> dst, bool isDepth) {
        if (!src || !dst || src->vkImage == VK_NULL_HANDLE ||
            dst->vkImage == VK_NULL_HANDLE) {
            return;
        }

        if (src->width == 0 || src->height == 0 || dst->width == 0 ||
            dst->height == 0) {
            return;
        }

        uint32_t layerCount =
            (src->type == TextureType::TextureCubeMap) ? 6 : 1;
        uint32_t dstLayers = (dst->type == TextureType::TextureCubeMap) ? 6 : 1;
        layerCount = std::min(layerCount, dstLayers);
        // These resolved/gbuffer textures are typically sampled later in the
        // frame (SSAO/lighting/SSR/debug), so keep them in a readable layout.
        // Restoring the *source* back to an attachment layout causes
        // validation errors when the next pass samples it.
        VkImageLayout srcFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkImageLayout dstFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkImageAspectFlags aspect =
            aspectMaskForFormat(src->format, isDepth);

        cmdTransitionImageLayout(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 aspect, layerCount);
        cmdTransitionImageLayout(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 aspectMaskForFormat(dst->format, isDepth),
                                 layerCount);

        if (!isDepth && src->samples > 1 && dst->samples == 1) {
            VkImageResolve region{};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = layerCount;
            region.srcOffset = {.x = 0, .y = 0, .z = 0};
            region.dstSubresource = region.srcSubresource;
            region.dstOffset = {.x = 0, .y = 0, .z = 0};
            region.extent = {.width = static_cast<uint32_t>(dst->width),
                             .height = static_cast<uint32_t>(dst->height),
                             .depth = 1};

            vkCmdResolveImage(commandBuffer, src->vkImage,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              dst->vkImage,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        } else if (src->samples == dst->samples) {
            VkImageBlit region{};
            region.srcSubresource.aspectMask = aspect;
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = layerCount;
            region.dstSubresource.aspectMask =
                aspectMaskForFormat(dst->format, isDepth);
            region.dstSubresource.mipLevel = 0;
            region.dstSubresource.baseArrayLayer = 0;
            region.dstSubresource.layerCount = layerCount;
            region.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
            region.srcOffsets[1] = {.x = static_cast<int32_t>(src->width),
                                    .y = static_cast<int32_t>(src->height),
                                    .z = 1};
            region.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
            region.dstOffsets[1] = {.x = static_cast<int32_t>(dst->width),
                                    .y = static_cast<int32_t>(dst->height),
                                    .z = 1};

            VkFilter filter = isDepth ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
            vkCmdBlitImage(commandBuffer, src->vkImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->vkImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region,
                           filter);
        } else {
            cmdTransitionImageLayout(dst, dstFinalLayout,
                                     aspectMaskForFormat(dst->format, isDepth),
                                     layerCount);
            cmdTransitionImageLayout(src, srcFinalLayout, aspect, layerCount);
            return;
        }

        cmdTransitionImageLayout(dst, dstFinalLayout,
                                 aspectMaskForFormat(dst->format, isDepth),
                                 layerCount);
        cmdTransitionImageLayout(src, srcFinalLayout, aspect, layerCount);
    };

    auto sourceColors =
        gatherColorAttachments(action->source, action->colorAttachmentIndex);
    auto destinationColors = gatherColorAttachments(
        action->destination, action->colorAttachmentIndex);

    if (action->resolveColor && !sourceColors.empty() &&
        !destinationColors.empty()) {
        size_t colorCount =
            std::min(sourceColors.size(), destinationColors.size());
        for (size_t i = 0; i < colorCount; ++i) {
            blitOrResolve(sourceColors[i], destinationColors[i], false);
        }
    }

    if (action->resolveDepth) {
        auto srcDepth = getDepthAttachment(action->source);
        auto dstDepth = getDepthAttachment(action->destination);
        if (srcDepth && dstDepth) {
            blitOrResolve(srcDepth, dstDepth, true);
        }
    }
#endif
}

} // namespace opal
