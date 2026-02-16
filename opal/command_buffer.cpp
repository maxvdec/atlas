//
// command_buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The Command Buffer implementation for drawing commands
// Copyright (c) 2025 maxvdec
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <opal/opal.h>
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef METAL
#include "metal_state.h"
#endif

namespace opal {

CommandBuffer::~CommandBuffer() {
#ifdef METAL
    metal::releaseCommandBufferState(this);
#endif
}

#ifdef METAL
namespace {

constexpr NS::UInteger kVertexStreamBufferIndex = 24;
constexpr NS::UInteger kInstanceStreamBufferIndex = 25;
constexpr float kIdentityInstanceMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

template <typename T> static inline T alignUp(T value, T alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

static std::vector<std::shared_ptr<Texture>>
collectColorAttachments(const std::shared_ptr<Framebuffer> &framebuffer) {
    std::vector<std::shared_ptr<Texture>> colors;
    if (framebuffer == nullptr) {
        return colors;
    }
    const int drawLimit = framebuffer->getDrawBufferCount();
    int colorIndex = 0;
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.type == Attachment::Type::Color &&
            attachment.texture != nullptr) {
            if (drawLimit >= 0 && colorIndex >= drawLimit) {
                break;
            }
            colors.push_back(attachment.texture);
            colorIndex++;
        }
    }
    return colors;
}

static std::shared_ptr<Texture>
collectDepthAttachment(const std::shared_ptr<Framebuffer> &framebuffer) {
    if (framebuffer == nullptr) {
        return nullptr;
    }
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.texture == nullptr) {
            continue;
        }
        if (attachment.type == Attachment::Type::Depth ||
            attachment.type == Attachment::Type::DepthStencil) {
            return attachment.texture;
        }
    }
    return nullptr;
}

static std::shared_ptr<Texture>
collectStencilAttachment(const std::shared_ptr<Framebuffer> &framebuffer) {
    if (framebuffer == nullptr) {
        return nullptr;
    }
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.texture == nullptr) {
            continue;
        }
        if (attachment.type == Attachment::Type::Stencil ||
            attachment.type == Attachment::Type::DepthStencil) {
            return attachment.texture;
        }
    }
    return nullptr;
}

static void ensureDefaultAuxiliaryTextures(Device *device, int width,
                                           int height) {
    if (device == nullptr || width <= 0 || height <= 0) {
        return;
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.brightTexture == nullptr ||
        deviceState.brightTexture->width != width ||
        deviceState.brightTexture->height != height) {
        deviceState.brightTexture =
            Texture::create(TextureType::Texture2D, TextureFormat::Rgba16F,
                            width, height, TextureDataFormat::Rgba, nullptr, 1);
    }

    if (deviceState.depthTexture == nullptr ||
        deviceState.depthTexture->width != width ||
        deviceState.depthTexture->height != height) {
        deviceState.depthTexture = Texture::create(
            TextureType::Texture2D, TextureFormat::Depth32F, width, height,
            TextureDataFormat::DepthComponent, nullptr, 1);
    }
}

static void updateLayerDrawableSize(Device *device, int width, int height) {
    if (device == nullptr || width <= 0 || height <= 0) {
        return;
    }
    auto &deviceState = metal::deviceState(device);
    if (deviceState.context == nullptr) {
        return;
    }
    auto &contextState = metal::contextState(deviceState.context);
    if (contextState.layer == nullptr) {
        return;
    }
    contextState.layer->setDrawableSize(
        CGSizeMake(static_cast<double>(width), static_cast<double>(height)));
}

static void configureColorAttachmentForClear(MTL::RenderPassDescriptor *pass,
                                             uint32_t colorCount,
                                             const float clearColor[4],
                                             bool clearRequested) {
    if (pass == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < colorCount; ++i) {
        auto *attachment = pass->colorAttachments()->object(i);
        if (attachment == nullptr || attachment->texture() == nullptr) {
            continue;
        }
        if (clearRequested) {
            attachment->setLoadAction(MTL::LoadActionClear);
            attachment->setClearColor(MTL::ClearColor::Make(
                clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
        } else {
            attachment->setLoadAction(MTL::LoadActionLoad);
        }
        attachment->setStoreAction(MTL::StoreActionStore);
    }
}

static void configureDepthAttachmentForClear(MTL::RenderPassDescriptor *pass,
                                             float clearDepth,
                                             bool clearRequested) {
    if (pass == nullptr) {
        return;
    }
    auto *depthAttachment = pass->depthAttachment();
    if (depthAttachment != nullptr && depthAttachment->texture() != nullptr) {
        depthAttachment->setLoadAction(clearRequested ? MTL::LoadActionClear
                                                      : MTL::LoadActionLoad);
        depthAttachment->setStoreAction(MTL::StoreActionStore);
        depthAttachment->setClearDepth(static_cast<double>(clearDepth));
    }
    auto *stencilAttachment = pass->stencilAttachment();
    if (stencilAttachment != nullptr &&
        stencilAttachment->texture() != nullptr) {
        stencilAttachment->setLoadAction(clearRequested ? MTL::LoadActionClear
                                                        : MTL::LoadActionLoad);
        stencilAttachment->setStoreAction(MTL::StoreActionStore);
        stencilAttachment->setClearStencil(0);
    }
}

static uint32_t
requiredColorOutputs(const std::shared_ptr<Pipeline> &pipeline) {
    if (pipeline == nullptr || pipeline->shaderProgram == nullptr) {
        return 1;
    }
    auto &programState = metal::programState(pipeline->shaderProgram.get());
    return programState.fragmentColorOutputs > 0
               ? programState.fragmentColorOutputs
               : 1;
}

static MTL::RenderPipelineState *
getRenderPipelineState(Device *device,
                       const std::shared_ptr<Pipeline> &pipeline,
                       const std::array<MTL::PixelFormat, 8> &colorFormats,
                       uint32_t colorCount, MTL::PixelFormat depthFormat,
                       MTL::PixelFormat stencilFormat, uint32_t sampleCount) {
    if (device == nullptr || pipeline == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return nullptr;
    }

    auto &deviceState = metal::deviceState(device);
    auto &pipelineState = metal::pipelineState(pipeline.get());
    auto &programState = metal::programState(pipeline->shaderProgram.get());
    if (deviceState.device == nullptr ||
        programState.vertexFunction == nullptr ||
        programState.fragmentFunction == nullptr) {
        return nullptr;
    }

    std::string key = metal::makePipelineKey(
        colorFormats, colorCount, depthFormat, stencilFormat, sampleCount);
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.blendingEnabled));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendSrc));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendDst));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendOp));
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.depthTestEnabled));
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.depthWriteEnabled));
    key += "|" + std::to_string(static_cast<int>(pipelineState.depthCompare));
    auto cacheIt = pipelineState.renderPipelineCache.find(key);
    if (cacheIt != pipelineState.renderPipelineCache.end()) {
        return cacheIt->second;
    }

    MTL::RenderPipelineDescriptor *descriptor =
        MTL::RenderPipelineDescriptor::alloc()->init();
    descriptor->setVertexFunction(programState.vertexFunction);
    descriptor->setFragmentFunction(programState.fragmentFunction);
    descriptor->setSampleCount(std::max<uint32_t>(1, sampleCount));
    if (pipelineState.vertexDescriptor != nullptr) {
        descriptor->setVertexDescriptor(pipelineState.vertexDescriptor);
    }

    for (uint32_t i = 0; i < colorCount && i < colorFormats.size(); ++i) {
        auto *attachment = descriptor->colorAttachments()->object(i);
        if (attachment == nullptr) {
            continue;
        }
        attachment->setPixelFormat(colorFormats[i]);
        attachment->setWriteMask(MTL::ColorWriteMaskAll);
        attachment->setBlendingEnabled(pipelineState.blendingEnabled);
        if (pipelineState.blendingEnabled) {
            attachment->setRgbBlendOperation(pipelineState.blendOp);
            attachment->setAlphaBlendOperation(pipelineState.blendOp);
            attachment->setSourceRGBBlendFactor(pipelineState.blendSrc);
            attachment->setDestinationRGBBlendFactor(pipelineState.blendDst);
            attachment->setSourceAlphaBlendFactor(pipelineState.blendSrc);
            attachment->setDestinationAlphaBlendFactor(pipelineState.blendDst);
        }
    }

    if (depthFormat != MTL::PixelFormatInvalid) {
        descriptor->setDepthAttachmentPixelFormat(depthFormat);
    }
    if (stencilFormat != MTL::PixelFormatInvalid) {
        descriptor->setStencilAttachmentPixelFormat(stencilFormat);
    }

    NS::Error *error = nullptr;
    MTL::RenderPipelineState *created =
        deviceState.device->newRenderPipelineState(descriptor, &error);
    descriptor->release();
    if (created == nullptr) {
        std::string message = "Failed to create Metal render pipeline";
        if (error != nullptr && error->localizedDescription() != nullptr) {
            message += ": ";
            message += error->localizedDescription()->utf8String();
        }
        throw std::runtime_error(message);
    }

    pipelineState.renderPipelineCache[key] = created;
    return created;
}

static void uploadUniformBuffers(const std::shared_ptr<Pipeline> &pipeline,
                                 MTL::RenderCommandEncoder *encoder,
                                 MTL::Device *device) {
    if (pipeline == nullptr || encoder == nullptr || device == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return;
    }

    auto &programState = metal::programState(pipeline->shaderProgram.get());
    auto &pipelineState = metal::pipelineState(pipeline.get());

    for (const auto &binding : programState.bindings) {
        auto uploadStage = [&](bool fragmentStage) {
            uint32_t key = metal::stageBindingKey(binding.index, fragmentStage);
            auto &bytes = pipelineState.uniformData[key];
            size_t requiredSize = 0;
            auto bindingSizeIt = programState.bindingSize.find(key);
            if (bindingSizeIt != programState.bindingSize.end()) {
                requiredSize = bindingSizeIt->second;
            }
            if (bytes.size() < requiredSize) {
                bytes.resize(requiredSize, 0);
            }
            if (bytes.empty()) {
                return;
            }

            auto &uniformBuffer = pipelineState.uniformBuffers[key];
            if (uniformBuffer == nullptr ||
                uniformBuffer->length() <
                    static_cast<NS::UInteger>(bytes.size())) {
                if (uniformBuffer != nullptr) {
                    uniformBuffer->release();
                }
                uniformBuffer = device->newBuffer(
                    static_cast<NS::UInteger>(
                        alignUp(bytes.size(), static_cast<size_t>(16))),
                    MTL::ResourceStorageModeShared);
            }
            std::memcpy(uniformBuffer->contents(), bytes.data(), bytes.size());
            uniformBuffer->didModifyRange(
                NS::Range::Make(0, static_cast<NS::UInteger>(bytes.size())));

            if (fragmentStage) {
                encoder->setFragmentBuffer(uniformBuffer, 0, binding.index);
            } else {
                encoder->setVertexBuffer(uniformBuffer, 0, binding.index);
            }
        };

        if (binding.vertexStage) {
            uploadStage(false);
        }
        if (binding.fragmentStage) {
            uploadStage(true);
        }
    }
}

static void bindTextures(CommandBuffer *commandBuffer,
                         const std::shared_ptr<Pipeline> &pipeline,
                         MTL::RenderCommandEncoder *encoder,
                         MTL::Device *device) {
    if (commandBuffer == nullptr || pipeline == nullptr || encoder == nullptr ||
        device == nullptr) {
        return;
    }

    auto &commandState = metal::commandBufferState(commandBuffer);
    auto &pipelineState = metal::pipelineState(pipeline.get());
    std::array<MTL::Texture *, 32> desiredTextures{};
    std::array<MTL::SamplerState *, 32> desiredSamplers{};
    desiredTextures.fill(nullptr);
    desiredSamplers.fill(nullptr);

    for (const auto &pair : pipelineState.texturesByUnit) {
        int unit = pair.first;
        const auto &texture = pair.second;
        if (unit < 0 || unit >= static_cast<int>(desiredTextures.size()) ||
            texture == nullptr) {
            continue;
        }
        auto &textureState = metal::textureState(texture.get());
        if (textureState.texture == nullptr) {
            continue;
        }
        if (textureState.sampler == nullptr) {
            metal::rebuildTextureSampler(texture.get(), device);
        }
        desiredTextures[static_cast<size_t>(unit)] = textureState.texture;
        desiredSamplers[static_cast<size_t>(unit)] = textureState.sampler;
    }

    if (!commandState.textureBindingsInitialized) {
        commandState.boundVertexTextures.fill(nullptr);
        commandState.boundFragmentTextures.fill(nullptr);
        commandState.boundVertexSamplers.fill(nullptr);
        commandState.boundFragmentSamplers.fill(nullptr);
        commandState.textureBindingsInitialized = true;
    }

    for (size_t unit = 0; unit < desiredTextures.size(); ++unit) {
        MTL::Texture *desiredTexture = desiredTextures[unit];
        MTL::SamplerState *desiredSampler = desiredSamplers[unit];

        if (commandState.boundVertexTextures[unit] != desiredTexture) {
            encoder->setVertexTexture(desiredTexture,
                                      static_cast<NS::UInteger>(unit));
            commandState.boundVertexTextures[unit] = desiredTexture;
        }
        if (commandState.boundFragmentTextures[unit] != desiredTexture) {
            encoder->setFragmentTexture(desiredTexture,
                                        static_cast<NS::UInteger>(unit));
            commandState.boundFragmentTextures[unit] = desiredTexture;
        }
        if (commandState.boundVertexSamplers[unit] != desiredSampler) {
            encoder->setVertexSamplerState(desiredSampler,
                                           static_cast<NS::UInteger>(unit));
            commandState.boundVertexSamplers[unit] = desiredSampler;
        }
        if (commandState.boundFragmentSamplers[unit] != desiredSampler) {
            encoder->setFragmentSamplerState(desiredSampler,
                                             static_cast<NS::UInteger>(unit));
            commandState.boundFragmentSamplers[unit] = desiredSampler;
        }
    }
}

static void ensureRenderEncoder(CommandBuffer *commandBuffer, Device *device,
                                const std::shared_ptr<Framebuffer> &framebuffer,
                                const std::shared_ptr<Pipeline> &boundPipeline,
                                const float clearColorValue[4],
                                float clearDepthValue) {
    if (commandBuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(commandBuffer);
    if (device == nullptr || framebuffer == nullptr ||
        boundPipeline == nullptr) {
        return;
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.queue == nullptr || deviceState.device == nullptr) {
        throw std::runtime_error("Metal device queue is not initialized");
    }
    if (state.passDescriptor == nullptr) {
        return;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }

    uint32_t requiredColors = requiredColorOutputs(boundPipeline);
    const int drawLimit = framebuffer->getDrawBufferCount();
    if (drawLimit >= 0) {
        requiredColors =
            std::min(requiredColors, static_cast<uint32_t>(drawLimit));
    }
    auto framebufferColors = collectColorAttachments(framebuffer);
    uint32_t actualColorCount = static_cast<uint32_t>(framebufferColors.size());
    uint32_t existingColorCount = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        auto *attachment = state.passDescriptor->colorAttachments()->object(i);
        if (attachment != nullptr && attachment->texture() != nullptr) {
            existingColorCount = std::max(existingColorCount, i + 1);
        }
    }

    int fbWidth = framebuffer->width;
    int fbHeight = framebuffer->height;
    if (framebuffer->isDefaultFramebuffer) {
        fbWidth = std::max(1, deviceState.drawableWidth);
        fbHeight = std::max(1, deviceState.drawableHeight);
        ensureDefaultAuxiliaryTextures(device, fbWidth, fbHeight);
    }

    uint32_t targetColorCount = std::max(actualColorCount, existingColorCount);
    if (framebuffer->isDefaultFramebuffer) {
        targetColorCount = std::max(targetColorCount, requiredColors);
        targetColorCount = std::max<uint32_t>(1, targetColorCount);
    }
    targetColorCount = std::min<uint32_t>(8, targetColorCount);

    for (uint32_t i = 0; i < targetColorCount; ++i) {
        auto *attachment = state.passDescriptor->colorAttachments()->object(i);
        if (attachment == nullptr) {
            continue;
        }
        if (attachment->texture() == nullptr) {
            if (i == 0 && framebuffer->isDefaultFramebuffer &&
                state.drawable != nullptr) {
                attachment->setTexture(state.drawable->texture());
                attachment->setStoreAction(MTL::StoreActionStore);
            } else if (framebuffer->isDefaultFramebuffer &&
                       deviceState.brightTexture != nullptr) {
                auto &brightState =
                    metal::textureState(deviceState.brightTexture.get());
                attachment->setTexture(brightState.texture);
                attachment->setStoreAction(MTL::StoreActionDontCare);
            }
        }
    }

    if (state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, targetColorCount,
                                         clearColorValue,
                                         state.clearColorPending);
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         state.clearDepthPending);
    }

    std::array<MTL::PixelFormat, 8> colorFormats{};
    colorFormats.fill(MTL::PixelFormatInvalid);
    uint32_t colorCount = 0;
    uint32_t sampleCount = 1;

    for (uint32_t i = 0; i < targetColorCount; ++i) {
        auto *attachment = state.passDescriptor->colorAttachments()->object(i);
        if (attachment == nullptr || attachment->texture() == nullptr) {
            continue;
        }
        colorFormats[i] = attachment->texture()->pixelFormat();
        colorCount = std::max(colorCount, i + 1);
        sampleCount = std::max<uint32_t>(
            sampleCount,
            static_cast<uint32_t>(attachment->texture()->sampleCount()));
    }

    MTL::PixelFormat depthFormat = MTL::PixelFormatInvalid;
    MTL::PixelFormat stencilFormat = MTL::PixelFormatInvalid;
    if (state.passDescriptor->depthAttachment() != nullptr &&
        state.passDescriptor->depthAttachment()->texture() != nullptr) {
        depthFormat =
            state.passDescriptor->depthAttachment()->texture()->pixelFormat();
        sampleCount = std::max<uint32_t>(
            sampleCount,
            static_cast<uint32_t>(state.passDescriptor->depthAttachment()
                                      ->texture()
                                      ->sampleCount()));
    }
    if (state.passDescriptor->stencilAttachment() != nullptr &&
        state.passDescriptor->stencilAttachment()->texture() != nullptr) {
        stencilFormat =
            state.passDescriptor->stencilAttachment()->texture()->pixelFormat();
    }

    if (state.encoder == nullptr) {
        state.encoder =
            state.commandBuffer->renderCommandEncoder(state.passDescriptor);
        if (state.encoder == nullptr) {
            throw std::runtime_error(
                "Failed to create Metal render command encoder");
        }

        state.clearColorPending = false;
        state.clearDepthPending = false;
    }

    MTL::RenderPipelineState *renderPipelineState =
        getRenderPipelineState(device, boundPipeline, colorFormats, colorCount,
                               depthFormat, stencilFormat, sampleCount);
    if (renderPipelineState == nullptr) {
        throw std::runtime_error("Metal render pipeline state creation failed");
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->setRenderPipelineState(renderPipelineState);
    if (pipelineState.depthStencilState != nullptr) {
        state.encoder->setDepthStencilState(pipelineState.depthStencilState);
    }
    state.encoder->setCullMode(pipelineState.cullMode);
    state.encoder->setFrontFacingWinding(pipelineState.frontFace);
    state.encoder->setTriangleFillMode(pipelineState.fillMode);
    if (pipelineState.polygonOffsetEnabled) {
        state.encoder->setDepthBias(pipelineState.polygonOffsetUnits,
                                    pipelineState.polygonOffsetFactor, 0.0f);
    } else {
        state.encoder->setDepthBias(0.0f, 0.0f, 0.0f);
    }

    int viewportX = std::max(0, pipelineState.viewportX);
    int viewportY = std::max(0, pipelineState.viewportY);
    int viewportW =
        pipelineState.viewportWidth > 0 ? pipelineState.viewportWidth : fbWidth;
    int viewportH = pipelineState.viewportHeight > 0
                        ? pipelineState.viewportHeight
                        : fbHeight;
    viewportW =
        std::max(1, std::min(viewportW, std::max(1, fbWidth - viewportX)));
    viewportH =
        std::max(1, std::min(viewportH, std::max(1, fbHeight - viewportY)));

    MTL::Viewport viewport{static_cast<double>(viewportX),
                           static_cast<double>(viewportY),
                           static_cast<double>(viewportW),
                           static_cast<double>(viewportH),
                           0.0,
                           1.0};
    state.encoder->setViewport(viewport);

    MTL::ScissorRect scissor{static_cast<NS::UInteger>(viewportX),
                             static_cast<NS::UInteger>(viewportY),
                             static_cast<NS::UInteger>(std::max(1, viewportW)),
                             static_cast<NS::UInteger>(std::max(1, viewportH))};
    state.encoder->setScissorRect(scissor);

    uploadUniformBuffers(boundPipeline, state.encoder, deviceState.device);
    bindTextures(commandBuffer, boundPipeline, state.encoder,
                 deviceState.device);
}

} // namespace
#endif

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
    vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(device->logicalDevice, 1, &inFlightFences[currentFrame]);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    if (state.autoreleasePool != nullptr) {
        state.autoreleasePool->release();
    }
    state.autoreleasePool = NS::AutoreleasePool::alloc()->init();
    state.commandBuffer = nullptr;
    state.encoder = nullptr;
    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
    }
    state.passDescriptor = nullptr;
    state.drawable = nullptr;
    state.boundVertexTextures.fill(nullptr);
    state.boundFragmentTextures.fill(nullptr);
    state.boundVertexSamplers.fill(nullptr);
    state.boundFragmentSamplers.fill(nullptr);
    state.textureBindingsInitialized = false;
    state.needsPresent = false;
    state.hasDraw = false;
    state.clearColorPending = false;
    state.clearDepthPending = false;
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
        framebuffer->width = static_cast<int>(device->swapChainExtent.width);
        framebuffer->height = static_cast<int>(device->swapChainExtent.height);
    }
#elif defined(METAL)
    if (device == nullptr) {
        throw std::runtime_error("Metal command buffer has no device");
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.device == nullptr || deviceState.queue == nullptr) {
        throw std::runtime_error("Metal device queue is not initialized");
    }

    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }

    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }

    state.passDescriptor =
        MTL::RenderPassDescriptor::renderPassDescriptor()->copy();
    state.hasDraw = false;

    if (framebuffer->isDefaultFramebuffer) {
        if (deviceState.context == nullptr) {
            throw std::runtime_error("Metal device context is missing");
        }
        GLFWwindow *window = deviceState.context->getWindow();
        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        fbWidth = std::max(1, fbWidth);
        fbHeight = std::max(1, fbHeight);

        updateLayerDrawableSize(device, fbWidth, fbHeight);
        ensureDefaultAuxiliaryTextures(device, fbWidth, fbHeight);

        auto &contextState = metal::contextState(deviceState.context);
        deviceState.drawable = contextState.layer->nextDrawable();
        state.drawable = deviceState.drawable;
        if (deviceState.drawable == nullptr) {
            state.passDescriptor->release();
            state.passDescriptor = nullptr;
            state.commandBuffer = nullptr;
            state.needsPresent = false;
            return;
        }
        deviceState.drawableWidth = fbWidth;
        deviceState.drawableHeight = fbHeight;
        framebuffer->width = fbWidth;
        framebuffer->height = fbHeight;

        auto *color0 = state.passDescriptor->colorAttachments()->object(0);
        color0->setTexture(deviceState.drawable->texture());
        color0->setStoreAction(MTL::StoreActionStore);

        if (deviceState.depthTexture != nullptr) {
            auto &depthState =
                metal::textureState(deviceState.depthTexture.get());
            auto *depthAttachment = state.passDescriptor->depthAttachment();
            depthAttachment->setTexture(depthState.texture);
            depthAttachment->setStoreAction(MTL::StoreActionStore);
            if (depthState.format == TextureFormat::Depth24Stencil8) {
                auto *stencilAttachment =
                    state.passDescriptor->stencilAttachment();
                stencilAttachment->setTexture(depthState.texture);
                stencilAttachment->setStoreAction(MTL::StoreActionStore);
            }
        }

        state.needsPresent = true;
    } else {
        std::vector<std::shared_ptr<Texture>> colors =
            collectColorAttachments(framebuffer);
        for (size_t i = 0; i < colors.size() && i < 8; ++i) {
            auto *attachment =
                state.passDescriptor->colorAttachments()->object(i);
            auto &textureState = metal::textureState(colors[i].get());
            attachment->setTexture(textureState.texture);
            attachment->setStoreAction(MTL::StoreActionStore);
        }

        auto depthTexture = collectDepthAttachment(framebuffer);
        if (depthTexture != nullptr) {
            auto *depthAttachment = state.passDescriptor->depthAttachment();
            auto &depthState = metal::textureState(depthTexture.get());
            depthAttachment->setTexture(depthState.texture);
            depthAttachment->setStoreAction(MTL::StoreActionStore);
        }

        auto stencilTexture = collectStencilAttachment(framebuffer);
        if (stencilTexture != nullptr) {
            auto *stencilAttachment = state.passDescriptor->stencilAttachment();
            auto &stencilState = metal::textureState(stencilTexture.get());
            stencilAttachment->setTexture(stencilState.texture);
            stencilAttachment->setStoreAction(MTL::StoreActionStore);
        }
    }

    configureColorAttachmentForClear(state.passDescriptor, 8, clearColorValue,
                                     state.clearColorPending);
    configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                     state.clearDepthPending);
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

        if (framebuffer != nullptr && !framebuffer->isDefaultFramebuffer) {
            for (auto &attachment : framebuffer->attachments) {
                if (attachment.texture != nullptr) {
                    if (attachment.type == opal::Attachment::Type::Color) {
                        attachment.texture->currentLayout =
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    } else {
                        attachment.texture->currentLayout =
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }
                }
            }
        }
    }
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
        state.passDescriptor = nullptr;
    }
    state.textureBindingsInitialized = false;
    state.hasDraw = false;
#endif
}

int CommandBuffer::getAndResetDrawCallCount() {
    int count = drawCallCount;
    drawCallCount = 0;
    return count;
}

void CommandBuffer::commit() {
#ifdef VULKAN
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
            std::cout << "commit(): Skipping frame - no render pass!"
                      << std::endl;
            return;
        }

        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;

        beginCommandBufferIfNeeded();
        this->record(imageIndex);
        hasStarted = true;
    }

    if (!imageAcquired) {
        return;
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
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    if (state.commandBuffer == nullptr) {
        if (state.passDescriptor != nullptr) {
            state.passDescriptor->release();
            state.passDescriptor = nullptr;
        }
        if (state.autoreleasePool != nullptr) {
            state.autoreleasePool->release();
            state.autoreleasePool = nullptr;
        }
        state.textureBindingsInitialized = false;
        return;
    }

    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }

    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
        state.passDescriptor = nullptr;
    }

    if (state.needsPresent && state.drawable != nullptr) {
        state.commandBuffer->presentDrawable(state.drawable);
    }

    state.commandBuffer->commit();
    state.commandBuffer->waitUntilCompleted();
    state.commandBuffer = nullptr;
    state.passDescriptor = nullptr;
    state.drawable = nullptr;
    state.textureBindingsInitialized = false;
    state.needsPresent = false;
    state.hasDraw = false;
    if (state.autoreleasePool != nullptr) {
        state.autoreleasePool->release();
        state.autoreleasePool = nullptr;
    }
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

        if (framebufferMatch && *corePipeline->opalPipeline == pipeline) {
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
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
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
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, instanceCount,
              firstVertex, firstInstance);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->instanceBuffer != nullptr) {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    } else {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawPrimitives(pipelineState.primitiveType,
                                  static_cast<NS::UInteger>(firstVertex),
                                  static_cast<NS::UInteger>(vertexCount),
                                  static_cast<NS::UInteger>(instanceCount),
                                  static_cast<NS::UInteger>(firstInstance));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = (int)device->frameCount;
        info.type = DrawCallType::Draw;
        info.send();
    }

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
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    if (boundDrawingState != nullptr) {
        bindVertexBuffersIfNeeded();
        if (boundDrawingState->indexBuffer != nullptr) {
            vkCmdBindIndexBuffer(commandBuffers[currentFrame],
                                 boundDrawingState->indexBuffer->vkBuffer, 0,
                                 VK_INDEX_TYPE_UINT32);
        }
    }
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
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
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, instanceCount,
                     firstIndex, vertexOffset, firstInstance);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }
    if (boundDrawingState == nullptr ||
        boundDrawingState->indexBuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }

    if (boundDrawingState->instanceBuffer != nullptr) {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    } else {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    }

    auto &indexState = metal::bufferState(boundDrawingState->indexBuffer.get());
    if (indexState.buffer == nullptr) {
        return;
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawIndexedPrimitives(
        pipelineState.primitiveType, static_cast<NS::UInteger>(indexCount),
        MTL::IndexTypeUInt32, indexState.buffer,
        static_cast<NS::UInteger>(firstIndex * sizeof(uint)),
        static_cast<NS::UInteger>(instanceCount), vertexOffset,
        static_cast<NS::UInteger>(firstInstance));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = (int)device->frameCount;
        info.type = DrawCallType::Indexed;
        info.send();
    }

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
                framebuffer->vkFramebuffers.resize(
                    device->swapChainImages.imageViews.size());
                if (device->swapChainBrightTextures.size() !=
                    device->swapChainImages.imageViews.size()) {
                    device->createSwapChainBrightTextures();
                }
                for (size_t i = 0;
                     i < device->swapChainImages.imageViews.size(); i++) {
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
                    framebufferInfo.renderPass =
                        renderPass->currentRenderPass->renderPass;
                    framebufferInfo.attachmentCount =
                        static_cast<uint32_t>(attachments.size());
                    framebufferInfo.pAttachments = attachments.data();
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
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
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
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, 1, firstVertex, 0);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }
    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }
    if (boundDrawingState == nullptr ||
        boundDrawingState->instanceBuffer == nullptr) {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    } else {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    }
    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawPrimitives(pipelineState.primitiveType,
                                  static_cast<NS::UInteger>(firstVertex),
                                  static_cast<NS::UInteger>(vertexCount));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = device->frameCount;
        info.type = DrawCallType::Patch;
        info.send();
    }

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
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearColorPending = true;
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, 8,
                                         clearColorValue, true);
    }
#endif
}

void CommandBuffer::clearDepth(float depth) {
#ifdef OPENGL
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearDepthPending = true;
#endif
    this->clearDepthValue = depth;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         true);
    }
#endif
}

void CommandBuffer::clear(float r, float g, float b, float a, float depth) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearColorPending = true;
    state.clearDepthPending = true;
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;
    this->clearDepthValue = depth;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, 8,
                                         clearColorValue, true);
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         true);
    }
#endif
}

} // namespace opal
