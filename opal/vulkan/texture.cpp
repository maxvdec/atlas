//
// texture.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Vulkan texture implementation
// Copyright (c) 2025 maxvdec
//

#ifdef VULKAN
#include <opal/opal.h>
#include <cstring>
#include <vulkan/vulkan.hpp>

namespace opal {

// Forward declarations
static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                              uint32_t height, uint32_t layerCount);
static void copyBufferToImage3D(VkBuffer buffer, VkImage image, uint32_t width,
                                uint32_t height, uint32_t depth);

VkFormat opalTextureFormatToVulkanFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::Rgba8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::sRgba8:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::Rgb8:
        // 3-channel formats not widely supported, use 4-channel
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::sRgb8:
        // 3-channel formats not widely supported, use 4-channel
        return VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::Rgba16F:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::Rgb16F:
        // VK_FORMAT_R16G16B16_SFLOAT not supported on most GPUs
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::Depth24Stencil8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case TextureFormat::DepthComponent24:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case TextureFormat::Depth32F:
        return VK_FORMAT_D32_SFLOAT;
    case TextureFormat::Red8:
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::Red16F:
        return VK_FORMAT_R16_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageType getVkImageType(TextureType type) {
    switch (type) {
    case TextureType::Texture2D:
    case TextureType::TextureCubeMap:
    case TextureType::Texture2DArray:
    case TextureType::Texture2DMultisample:
        return VK_IMAGE_TYPE_2D;
    case TextureType::Texture3D:
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_2D;
    }
}

static VkImageViewType getVkImageViewType(TextureType type) {
    switch (type) {
    case TextureType::Texture2D:
    case TextureType::Texture2DMultisample:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType::TextureCubeMap:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TextureType::Texture3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureType::Texture2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

static VkImageUsageFlags getVkImageUsageFlags(TextureFormat format) {
    VkImageUsageFlags flags = VK_IMAGE_USAGE_SAMPLED_BIT;

    switch (format) {
    case TextureFormat::Depth24Stencil8:
    case TextureFormat::DepthComponent24:
    case TextureFormat::Depth32F:
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    }

    return flags;
}

static VkImageAspectFlags getVkAspectFlags(TextureFormat format) {
    switch (format) {
    case TextureFormat::Depth24Stencil8:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    case TextureFormat::DepthComponent24:
    case TextureFormat::Depth32F:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static void createImage(int width, int height, int depth, uint32_t arrayLayers,
                        VkFormat format, VkImageType imageType,
                        VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties, VkImage &image,
                        VkDeviceMemory &imageMemory,
                        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                        VkImageCreateFlags flags = 0) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = static_cast<uint32_t>(depth);
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = samples;
    imageInfo.flags = flags;

    if (vkCreateImage(Device::globalDevice, &imageInfo, nullptr, &image) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(Device::globalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Device::globalInstance->findMemoryType(
        memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(Device::globalDevice, &allocInfo, nullptr,
                         &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan image memory!");
    }

    vkBindImageMemory(Device::globalDevice, image, imageMemory, 0);
}

static VkImageView createImageView(VkImage image, VkFormat format,
                                   VkImageAspectFlags aspectFlags,
                                   VkImageViewType viewType,
                                   uint32_t layerCount = 1) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

    VkImageView imageView;
    if (vkCreateImageView(Device::globalDevice, &viewInfo, nullptr,
                          &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image view!");
    }

    return imageView;
}

static VkSampler createSampler(TextureFilterMode minFilter,
                               TextureFilterMode magFilter,
                               TextureWrapMode wrapS, TextureWrapMode wrapT,
                               TextureWrapMode wrapR) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Convert filter modes
    auto convertFilter = [](TextureFilterMode mode) -> VkFilter {
        switch (mode) {
        case TextureFilterMode::Nearest:
        case TextureFilterMode::NearestMipmapNearest:
            return VK_FILTER_NEAREST;
        case TextureFilterMode::Linear:
        case TextureFilterMode::LinearMipmapLinear:
        default:
            return VK_FILTER_LINEAR;
        }
    };

    auto convertMipmapMode = [](TextureFilterMode mode) -> VkSamplerMipmapMode {
        switch (mode) {
        case TextureFilterMode::NearestMipmapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case TextureFilterMode::LinearMipmapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    };

    auto convertWrap = [](TextureWrapMode mode) -> VkSamplerAddressMode {
        switch (mode) {
        case TextureWrapMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrapMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureWrapMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrapMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };

    samplerInfo.magFilter = convertFilter(magFilter);
    samplerInfo.minFilter = convertFilter(minFilter);
    samplerInfo.mipmapMode = convertMipmapMode(minFilter);
    samplerInfo.addressModeU = convertWrap(wrapS);
    samplerInfo.addressModeV = convertWrap(wrapT);
    samplerInfo.addressModeW = convertWrap(wrapR);
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler;
    if (vkCreateSampler(Device::globalDevice, &samplerInfo, nullptr,
                        &sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan sampler!");
    }

    return sampler;
}

std::shared_ptr<Texture>
Texture::createVulkan(TextureType type, TextureFormat format, int width,
                      int height, TextureDataFormat dataFormat,
                      const void *data, uint mipLevels) {
    auto texture = std::make_shared<Texture>();
    texture->type = type;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    VkFormat vkFormat = opalTextureFormatToVulkanFormat(format);
    VkImageType imageType = getVkImageType(type);
    VkImageViewType viewType = getVkImageViewType(type);
    VkImageUsageFlags usageFlags = getVkImageUsageFlags(format);
    VkImageAspectFlags aspectFlags = getVkAspectFlags(format);

    uint32_t arrayLayers = (type == TextureType::TextureCubeMap) ? 6 : 1;
    VkImageCreateFlags createFlags = (type == TextureType::TextureCubeMap)
                                         ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
                                         : 0;

    createImage(width, height, 1, arrayLayers, vkFormat, imageType,
                VK_IMAGE_TILING_OPTIMAL, usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->vkImage,
                texture->vkImageMemory, VK_SAMPLE_COUNT_1_BIT, createFlags);

    texture->vkImageView = createImageView(texture->vkImage, vkFormat,
                                           aspectFlags, viewType, arrayLayers);

    texture->vkSampler =
        createSampler(TextureFilterMode::Linear, TextureFilterMode::Linear,
                      TextureWrapMode::Repeat, TextureWrapMode::Repeat,
                      TextureWrapMode::Repeat);

    // If data is provided, upload it via a staging buffer
    if (data != nullptr && width > 0 && height > 0) {
        // Calculate size based on format
        VkDeviceSize imageSize = 0;
        switch (dataFormat) {
        case TextureDataFormat::Rgba:
            imageSize = width * height * 4;
            break;
        case TextureDataFormat::Rgb:
            imageSize = width * height * 3;
            break;
        case TextureDataFormat::Red:
            imageSize = width * height;
            break;
        default:
            imageSize = width * height * 4;
            break;
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        Buffer::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

        void *mappedData;
        vkMapMemory(Device::globalDevice, stagingBufferMemory, 0, imageSize, 0,
                    &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(imageSize));
        vkUnmapMemory(Device::globalDevice, stagingBufferMemory);

        // Transition image layout and copy
        Framebuffer::transitionImageLayout(
            texture->vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arrayLayers);

        // Copy buffer to image
        copyBufferToImage(stagingBuffer, texture->vkImage, width, height,
                          arrayLayers);

        Framebuffer::transitionImageLayout(
            texture->vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arrayLayers);

        texture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkDestroyBuffer(Device::globalDevice, stagingBuffer, nullptr);
        vkFreeMemory(Device::globalDevice, stagingBufferMemory, nullptr);
    }

    (void)mipLevels;
    return texture;
}

std::shared_ptr<Texture> Texture::createMultisampledVulkan(TextureFormat format,
                                                           int width,
                                                           int height,
                                                           int samples) {
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture2DMultisample;
    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->samples = samples;

    VkFormat vkFormat = opalTextureFormatToVulkanFormat(format);
    VkImageUsageFlags usageFlags = getVkImageUsageFlags(format);
    VkImageAspectFlags aspectFlags = getVkAspectFlags(format);

    VkSampleCountFlagBits sampleCount;
    switch (samples) {
    case 2:
        sampleCount = VK_SAMPLE_COUNT_2_BIT;
        break;
    case 4:
        sampleCount = VK_SAMPLE_COUNT_4_BIT;
        break;
    case 8:
        sampleCount = VK_SAMPLE_COUNT_8_BIT;
        break;
    case 16:
        sampleCount = VK_SAMPLE_COUNT_16_BIT;
        break;
    default:
        sampleCount = VK_SAMPLE_COUNT_4_BIT;
        break;
    }

    createImage(width, height, 1, 1, vkFormat, VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL, usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->vkImage,
                texture->vkImageMemory, sampleCount);

    texture->vkImageView = createImageView(
        texture->vkImage, vkFormat, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1);

    return texture;
}

std::shared_ptr<Texture> Texture::createDepthCubemapVulkan(TextureFormat format,
                                                           int resolution) {
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::TextureCubeMap;
    texture->format = format;
    texture->width = resolution;
    texture->height = resolution;

    VkFormat vkFormat = opalTextureFormatToVulkanFormat(format);
    VkImageAspectFlags aspectFlags = getVkAspectFlags(format);

    createImage(resolution, resolution, 1, 6, vkFormat, VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->vkImage,
                texture->vkImageMemory, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    texture->vkImageView = createImageView(
        texture->vkImage, vkFormat, aspectFlags, VK_IMAGE_VIEW_TYPE_CUBE, 6);

    texture->vkSampler = createSampler(
        TextureFilterMode::Nearest, TextureFilterMode::Nearest,
        TextureWrapMode::ClampToEdge, TextureWrapMode::ClampToEdge,
        TextureWrapMode::ClampToEdge);

    return texture;
}

std::shared_ptr<Texture>
Texture::create3DVulkan(TextureFormat format, int width, int height, int depth,
                        TextureDataFormat dataFormat, const void *data) {
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture3D;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    VkFormat vkFormat = opalTextureFormatToVulkanFormat(format);
    VkImageUsageFlags usageFlags =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createImage(width, height, depth, 1, vkFormat, VK_IMAGE_TYPE_3D,
                VK_IMAGE_TILING_OPTIMAL, usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->vkImage,
                texture->vkImageMemory);

    texture->vkImageView =
        createImageView(texture->vkImage, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT,
                        VK_IMAGE_VIEW_TYPE_3D, 1);

    texture->vkSampler = createSampler(
        TextureFilterMode::Linear, TextureFilterMode::Linear,
        TextureWrapMode::ClampToEdge, TextureWrapMode::ClampToEdge,
        TextureWrapMode::ClampToEdge);

    if (data != nullptr) {
        // Upload data similar to 2D textures
        VkDeviceSize imageSize = width * height * depth * 4; // Assuming RGBA
        if (dataFormat == TextureDataFormat::Red) {
            imageSize = width * height * depth;
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        Buffer::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingBufferMemory);

        void *mappedData;
        vkMapMemory(Device::globalDevice, stagingBufferMemory, 0, imageSize, 0,
                    &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(imageSize));
        vkUnmapMemory(Device::globalDevice, stagingBufferMemory);

        Framebuffer::transitionImageLayout(
            texture->vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage3D(stagingBuffer, texture->vkImage, width, height,
                            depth);

        Framebuffer::transitionImageLayout(
            texture->vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        texture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkDestroyBuffer(Device::globalDevice, stagingBuffer, nullptr);
        vkFreeMemory(Device::globalDevice, stagingBufferMemory, nullptr);
    }

    return texture;
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                       uint32_t height, uint32_t layerCount) {
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

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

void copyBufferToImage3D(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height, uint32_t depth) {
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

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, depth};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

} // namespace opal

#endif
