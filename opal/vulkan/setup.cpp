/*
 * setup.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2025
 * --------------------------------------
 * Description: Vulkan setup and initialization
 * Copyright (c) 2025 Max Van den Eynde
 */

#ifdef VULKAN
#include <opal/opal.h>
#include <vulkan/vulkan.hpp>
#include <glfw/glfw3.h>

namespace opal {
void Context::createInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = this->config.applicationName.c_str();
    appInfo.pEngineName = "Atlas Engine";
    appInfo.apiVersion = VK_API_VERSION_1_2;
    int major = std::stoi(this->config.applicationVersion.substr(
        0, this->config.applicationVersion.find('.')));
    int minor = std::stoi(this->config.applicationVersion.substr(
        this->config.applicationVersion.find('.') + 1,
        this->config.applicationVersion.rfind('.') -
            this->config.applicationVersion.find('.') - 1));
    int patch = std::stoi(this->config.applicationVersion.substr(
        this->config.applicationVersion.rfind('.') + 1));
    appInfo.applicationVersion = VK_MAKE_VERSION(major, minor, patch);
    appInfo.engineVersion = VK_MAKE_VERSION(0, 5, 0);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    const char **glfwExtensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char *> extensions;

    for (uint32_t i = 0; i < extensionCount; i++) {
        extensions.emplace_back(glfwExtensions[i]);
    }

    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

} // namespace opal

#endif