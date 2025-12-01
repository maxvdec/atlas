/*
 device.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Device and context functions
 Copyright (c) 2025 maxvdec
*/

#include "opal/opal.h"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

std::shared_ptr<Context> Context::create(ContextConfiguration config) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    auto context = std::make_shared<Context>();
    context->config = config;

#ifdef VULKAN
    context->createInstance();
    if (config.createValidationLayers) {
        context->setupMessenger();
    }
#endif

    if (config.useOpenGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.majorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config.minorVersion);
        if (config.profile == OpenGLProfile::Core) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        }
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    return context;
}

void Context::setFlag(int flag, bool enabled) {
    glfwWindowHint(flag, enabled ? GLFW_TRUE : GLFW_FALSE);
}

void Context::setFlag(int flag, int value) { glfwWindowHint(flag, value); }

void Context::makeCurrent() {
    if (this->window != nullptr) {
        glfwMakeContextCurrent(this->window);
    }
}

GLFWwindow *Context::makeWindow(int width, int height, const char *title,
                                GLFWmonitor *monitor, GLFWwindow *share) {
    this->window = glfwCreateWindow(width, height, title, monitor, share);
    if (this->window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
#ifdef VULKAN
    this->setupSurface();
#endif
    return this->window;
}

GLFWwindow *Context::getWindow() const {
    if (this->window == nullptr)
        throw std::runtime_error("Cannot obtain a window before created");
    return this->window;
}

std::shared_ptr<Device>
Device::acquire([[maybe_unused]] std::shared_ptr<Context> context) {
#ifdef OPENGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    auto device = std::make_shared<Device>();
    return device;
#else
    auto device = std::make_shared<Device>();
    device->context = context;
    device->pickPhysicalDevice(context);
    device->createLogicalDevice(context);
    device->createSwapChain(context);
    Device::globalInstance = device.get();
    return device;
#endif
}

std::shared_ptr<Framebuffer> Device::getDefaultFramebuffer() {
    if (defaultFramebuffer == nullptr) {
        defaultFramebuffer = std::make_shared<Framebuffer>();
        defaultFramebuffer->framebufferID = 0;
        defaultFramebuffer->width = 0;
        defaultFramebuffer->height = 0;
        defaultFramebuffer->isDefaultFramebuffer = true;
    }
    return defaultFramebuffer;
}

Device *Device::globalInstance = nullptr;
VkDevice Device::globalDevice = VK_NULL_HANDLE;

} // namespace opal
