//
// setup.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Setup the main vulkan parts
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/log.h"
#include <iostream>
#include <vector>
#ifdef VULKAN
#include <opal/opal.h>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

namespace opal {
void Context::createInstance() {
    if (this->config.createValidationLayers && !this->hasValidationLayer()) {
        throw std::runtime_error(
            "Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    if (!this->config.applicationName.empty()) {
        appInfo.pApplicationName = this->config.applicationName.c_str();
    } else {
        appInfo.pApplicationName = "Atlas Application";
    }

    appInfo.pEngineName = "Atlas Engine";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    if (!this->config.applicationVersion.empty()) {
        int major = std::stoi(this->config.applicationVersion.substr(
            0, this->config.applicationVersion.find('.')));
        int minor = std::stoi(this->config.applicationVersion.substr(
            this->config.applicationVersion.find('.') + 1,
            this->config.applicationVersion.rfind('.') -
                this->config.applicationVersion.find('.') - 1));
        int patch = std::stoi(this->config.applicationVersion.substr(
            this->config.applicationVersion.rfind('.') + 1));
        appInfo.applicationVersion = VK_MAKE_VERSION(major, minor, patch);
    } else {
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    }

    appInfo.engineVersion = VK_MAKE_VERSION(0, 5, 0);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = this->getExtensions();

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (this->config.createValidationLayers) {
        createInfo.enabledLayerCount = 1;
        const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
        createInfo.ppEnabledLayerNames = &validationLayerName;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

bool Context::hasValidationLayer() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    const char *validationLayerName = "VK_LAYER_KHRONOS_validation";

    for (const auto &layerProperties : availableLayers) {
        if (strcmp(layerProperties.layerName, validationLayerName) == 0) {
            return true;
        }
    }

    return false;
}

std::vector<const char *> Context::getExtensions() {
    uint32_t extensionCount = 0;
    const char **glfwExtensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char *> extensions;

    extensions.reserve(extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        extensions.emplace_back(glfwExtensions[i]);
    }

    if (this->config.createValidationLayers) {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Context::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        atlas_error(pCallbackData->pMessage);
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        atlas_warning(pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        atlas_log(pCallbackData->pMessage);
    }

    return VK_FALSE;
}

void Context::setupMessenger() {
    if (!this->config.createValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(this->instance,
                              "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        if (func(this->instance, &createInfo, nullptr, &this->debugMessenger) !=
            VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to set up debug messenger for Vulkan");
        }
    } else {
        throw std::runtime_error("Failed to set up debug messenger for Vulkan");
    }
}

void Context::setupSurface() {
    if (glfwCreateWindowSurface(this->instance, this->window, nullptr,
                                &this->surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

} // namespace opal

#endif