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
#ifdef METAL
#include "metal_state.h"
#endif
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

namespace {

void upsertColorAttachment(std::vector<Attachment> &attachments, int colorIndex,
                           const std::shared_ptr<Texture> &texture) {
    if (colorIndex < 0) {
        return;
    }

    int currentColor = 0;
    for (auto &attachment : attachments) {
        if (attachment.type != Attachment::Type::Color) {
            continue;
        }
        if (currentColor == colorIndex) {
            attachment.texture = texture;
            return;
        }
        currentColor++;
    }

    Attachment attachment;
    attachment.type = Attachment::Type::Color;
    attachment.texture = texture;
    attachments.push_back(attachment);
}

void upsertAttachmentByType(std::vector<Attachment> &attachments,
                            Attachment::Type type,
                            const std::shared_ptr<Texture> &texture) {
    for (auto &attachment : attachments) {
        if (attachment.type == type) {
            attachment.texture = texture;
            return;
        }
    }
    Attachment attachment;
    attachment.type = type;
    attachment.texture = texture;
    attachments.push_back(attachment);
}

std::vector<std::shared_ptr<Texture>>
collectDrawColorAttachments(const std::shared_ptr<Framebuffer> &fb,
                            int preferredIndex = -1) {
    std::vector<std::shared_ptr<Texture>> result;
    if (fb == nullptr) {
        return result;
    }

    const int drawLimit = fb->getDrawBufferCount();
    int colorIndex = 0;
    for (const auto &attachment : fb->attachments) {
        if (attachment.type != Attachment::Type::Color ||
            attachment.texture == nullptr) {
            continue;
        }
        if (drawLimit >= 0 && colorIndex >= drawLimit) {
            break;
        }
        if (preferredIndex >= 0) {
            if (colorIndex == preferredIndex) {
                result.push_back(attachment.texture);
                break;
            }
        } else {
            result.push_back(attachment.texture);
        }
        colorIndex++;
    }
    return result;
}

} // namespace

Framebuffer::~Framebuffer() {
#ifdef METAL
    metal::releaseFramebufferState(this);
#endif
}

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
    framebuffer->drawBufferCount = -1;

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
    framebuffer->drawBufferCount = -1;

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
#elif defined(VULKAN) || defined(METAL)
    upsertColorAttachment(attachments, attachmentIndex, texture);
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
#elif defined(VULKAN) || defined(METAL)
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
#elif defined(VULKAN) || defined(METAL)
    upsertAttachmentByType(attachments, attachmentType, texture);
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
#elif defined(VULKAN) || defined(METAL)
    (void)face;
    upsertAttachmentByType(attachments, attachmentType, texture);
#endif
}

void Framebuffer::disableColorBuffer() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    colorBufferDisabled = true;
#elif defined(VULKAN) || defined(METAL)
    colorBufferDisabled = true;
    drawBufferCount = 0;
#endif
}

void Framebuffer::setViewport() {
#ifdef OPENGL
    glViewport(0, 0, width, height);
#elif defined(VULKAN)
#elif defined(METAL)
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
#elif defined(METAL)
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
#elif defined(METAL)
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
#elif defined(METAL)
#endif
}

void Framebuffer::unbind() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(VULKAN)
#elif defined(METAL)
#endif
}

void Framebuffer::bindForRead() {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferID);
#elif defined(VULKAN)
#elif defined(METAL)
#endif
}

void Framebuffer::bindForDraw() {
#ifdef OPENGL
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferID);
#elif defined(VULKAN)
#elif defined(METAL)
#endif
}

void Framebuffer::setDrawBuffers(int attachmentCount) {
    drawBufferCount = attachmentCount < 0 ? -1 : attachmentCount;
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
#elif defined(METAL)
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
#elif defined(METAL)
    if (action == nullptr || action->source == nullptr ||
        action->destination == nullptr) {
        return;
    }
    if (Device::globalInstance == nullptr) {
        return;
    }

    auto &deviceState = metal::deviceState(Device::globalInstance);
    if (deviceState.queue == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }

    bool ownedCommandBuffer = false;
    NS::AutoreleasePool *resolvePool = nullptr;
    MTL::CommandBuffer *metalCB = state.commandBuffer;
    if (metalCB == nullptr) {
        resolvePool = NS::AutoreleasePool::alloc()->init();
        metalCB = deviceState.queue->commandBuffer();
        ownedCommandBuffer = true;
    }

    auto gatherColorAttachments = [](const std::shared_ptr<Framebuffer> &fb,
                                     int preferredIndex) {
        return collectDrawColorAttachments(fb, preferredIndex);
    };

    auto gatherDepthAttachment = [](const std::shared_ptr<Framebuffer> &fb) {
        if (fb == nullptr) {
            return std::shared_ptr<Texture>{nullptr};
        }
        for (const auto &attachment : fb->attachments) {
            if ((attachment.type == Attachment::Type::Depth ||
                 attachment.type == Attachment::Type::DepthStencil) &&
                attachment.texture != nullptr) {
                return attachment.texture;
            }
        }
        return std::shared_ptr<Texture>{nullptr};
    };

    auto resolveMsaaColor = [&](const std::shared_ptr<Texture> &source,
                                const std::shared_ptr<Texture> &destination) {
        auto &srcState = metal::textureState(source.get());
        auto &dstState = metal::textureState(destination.get());
        if (srcState.texture == nullptr || dstState.texture == nullptr) {
            return;
        }

        MTL::RenderPassDescriptor *descriptor =
            MTL::RenderPassDescriptor::renderPassDescriptor();
        auto *colorAttachment = descriptor->colorAttachments()->object(0);
        colorAttachment->setTexture(srcState.texture);
        colorAttachment->setResolveTexture(dstState.texture);
        colorAttachment->setLoadAction(MTL::LoadActionLoad);
        colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

        MTL::RenderCommandEncoder *encoder =
            metalCB->renderCommandEncoder(descriptor);
        if (encoder != nullptr) {
            encoder->endEncoding();
        }
    };

    auto copyTexture = [&](const std::shared_ptr<Texture> &source,
                           const std::shared_ptr<Texture> &destination) {
        auto &srcState = metal::textureState(source.get());
        auto &dstState = metal::textureState(destination.get());
        if (srcState.texture == nullptr || dstState.texture == nullptr) {
            return;
        }
        MTL::BlitCommandEncoder *blit = metalCB->blitCommandEncoder();
        blit->copyFromTexture(srcState.texture, dstState.texture);
        blit->endEncoding();
    };

    auto sourceColors =
        gatherColorAttachments(action->source, action->colorAttachmentIndex);
    auto destinationColors = gatherColorAttachments(
        action->destination, action->colorAttachmentIndex);

    if (action->resolveColor && !sourceColors.empty() &&
        !destinationColors.empty()) {
        size_t count = std::min(sourceColors.size(), destinationColors.size());
        for (size_t i = 0; i < count; ++i) {
            auto &src = sourceColors[i];
            auto &dst = destinationColors[i];
            if (!src || !dst) {
                continue;
            }
            if (src->samples > 1 && dst->samples == 1) {
                resolveMsaaColor(src, dst);
            } else {
                copyTexture(src, dst);
            }
        }
    }

    if (action->resolveDepth) {
        auto srcDepth = gatherDepthAttachment(action->source);
        auto dstDepth = gatherDepthAttachment(action->destination);
        if (srcDepth != nullptr && dstDepth != nullptr &&
            srcDepth->samples == dstDepth->samples) {
            copyTexture(srcDepth, dstDepth);
        }
    }

    if (ownedCommandBuffer) {
        metalCB->commit();
        metalCB->waitUntilCompleted();
        if (resolvePool != nullptr) {
            resolvePool->release();
            resolvePool = nullptr;
        }
    }
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

    auto beginOneTimeCommands = []() -> VkCommandBuffer {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = Device::globalInstance->commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(Device::globalDevice, &allocInfo,
                                     &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate resolve command "
                                     "buffer");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to begin resolve command buffer recording");
        }

        return commandBuffer;
    };

    auto endOneTimeCommands = [](VkCommandBuffer commandBuffer) {
        if (commandBuffer == VK_NULL_HANDLE) {
            return;
        }

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
    };

    auto gatherColorAttachments = [](std::shared_ptr<Framebuffer> fb,
                                     int preferredIndex) {
        return collectDrawColorAttachments(fb, preferredIndex);
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

    auto transitionTexture = [](std::shared_ptr<Texture> texture,
                                VkImageLayout newLayout, bool isAttachment,
                                bool isDepth, uint32_t layerCount) {
        if (!texture || texture->vkImage == VK_NULL_HANDLE) {
            return;
        }

        VkImageLayout currentLayout = texture->currentLayout;
        if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            if (isAttachment) {
                currentLayout =
                    isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            } else {
                currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
        }

        Framebuffer::transitionImageLayout(
            texture->vkImage, opalTextureFormatToVulkanFormat(texture->format),
            currentLayout, newLayout, layerCount);
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
        VkImageLayout srcFinalLayout =
            isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkImageLayout dstFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        transitionTexture(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, true,
                          isDepth, layerCount);
        transitionTexture(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false,
                          isDepth, layerCount);

        VkCommandBuffer commandBuffer = beginOneTimeCommands();

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
            VkImageAspectFlags aspect =
                aspectMaskForFormat(src->format, isDepth);
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
            endOneTimeCommands(commandBuffer);
            transitionTexture(dst, dstFinalLayout, false, isDepth, layerCount);
            transitionTexture(src, srcFinalLayout, true, isDepth, layerCount);
            return;
        }

        endOneTimeCommands(commandBuffer);

        transitionTexture(dst, dstFinalLayout, false, isDepth, layerCount);
        transitionTexture(src, srcFinalLayout, true, isDepth, layerCount);
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
