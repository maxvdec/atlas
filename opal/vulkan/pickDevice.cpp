//
// pickDevice.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Device picking for Vulkan backends
// Copyright (c) 2025 Max Van den Eynde
//
#include <cstdint>
#include <vector>
#include <set>
#ifdef VULKAN
#include <memory>
#include <opal/opal.h>
#include <vulkan/vulkan.hpp>
namespace opal {
bool Device::deviceMeetsRequirements(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!this->supportsDeviceExtension(device,
                                       VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return false;
    }

    return deviceProperties.deviceType ==
               VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader;
}

bool Device::supportsDeviceExtension(VkPhysicalDevice device,
                                     const char *extension) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());
    for (const auto &ext : availableExtensions) {
        if (strcmp(ext.extensionName, extension) == 0) {
            return true;
        }
    }
    return false;
}

void Device::pickPhysicalDevice(std::shared_ptr<Context> context) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context->instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices.data());
    for (const auto &device : devices) {
        if (this->deviceMeetsRequirements(device) &&
            this->findQueueFamilies(device, context->surface).isComplete()) {
            this->physicalDevice = device;
            break;
        }
    }
    if (this->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

Device::QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device,
                                                     VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

void Device::createLogicalDevice(std::shared_ptr<Context> context) {
    QueueFamilyIndices indices =
        findQueueFamilies(this->physicalDevice, context->surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (context->config.createValidationLayers) {
        createInfo.enabledLayerCount = 1;
        const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
        createInfo.ppEnabledLayerNames = &validationLayerName;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr,
                       &this->logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
    globalDevice = this->logicalDevice;

    vkGetDeviceQueue(this->logicalDevice, indices.graphicsFamily.value(), 0,
                     &this->graphicsQueue);
    vkGetDeviceQueue(this->logicalDevice, indices.presentFamily.value(), 0,
                     &this->presentQueue);
}

} // namespace opal
#endif