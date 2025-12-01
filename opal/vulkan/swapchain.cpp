//
// swapchain.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Swapchain implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include <cstddef>
#include <cstdint>
#include <memory>
#ifdef VULKAN
#include <opal/opal.h>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace opal {
Device::SwapChainSupportDetails
Device::querySwapChainSupport(VkPhysicalDevice device,
                              std::shared_ptr<Context> context) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context->surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, context->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface,
                                                  &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Device::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Device::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
Device::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                         GLFWwindow *window) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Device::createSwapChain(std::shared_ptr<Context> context) {
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(this->physicalDevice, context);

    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent =
        chooseSwapExtent(swapChainSupport.capabilities, context->window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices =
        findQueueFamilies(this->physicalDevice, context->surface);

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                     indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(this->logicalDevice, &createInfo, nullptr,
                             &this->swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(this->logicalDevice, this->swapChain, &imageCount,
                            nullptr);
    swapChainImages.images.resize(imageCount);
    vkGetSwapchainImagesKHR(this->logicalDevice, this->swapChain, &imageCount,
                            swapChainImages.images.data());

    this->swapChainExtent = extent;
    this->swapChainImageFormat = surfaceFormat.format;
}

void Device::createImageViews() {
    swapChainImages.imageViews.resize(swapChainImages.images.size());

    for (size_t i = 0; i < swapChainImages.images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(this->logicalDevice, &createInfo, nullptr,
                              &swapChainImages.imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }

    createSwapChainBrightTextures();
}

void Device::destroySwapChainBrightTextures() {
    swapChainBrightTextures.clear();
}

void Device::createSwapChainBrightTextures() {
    destroySwapChainBrightTextures();

    if (swapChainImages.images.empty()) {
        return;
    }

    swapChainBrightTextures.reserve(swapChainImages.images.size());
    for (size_t i = 0; i < swapChainImages.images.size(); ++i) {
        auto texture = Texture::create(TextureType::Texture2D,
                                       TextureFormat::Rgba16F,
                                       static_cast<int>(swapChainExtent.width),
                                       static_cast<int>(swapChainExtent.height),
                                       TextureDataFormat::Rgba, nullptr, 1);
        swapChainBrightTextures.push_back(texture);
    }
}

void Device::remakeSwapChain(std::shared_ptr<Context> context) {
    vkDeviceWaitIdle(this->logicalDevice);

    destroySwapChainBrightTextures();
    for (auto *imageView : swapChainImages.imageViews) {
        vkDestroyImageView(this->logicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(this->logicalDevice, this->swapChain, nullptr);

    this->createSwapChain(std::move(context));
    this->createImageViews();
}

} // namespace opal

#endif