/*
 device.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Device and context functions
 Copyright (c) 2025 maxvdec
*/

#include "opal/opal.h"
#include "atlas/tracer/log.h"
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
        atlas_error("Failed to initialize GLFW");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    atlas_log("Initializing graphics context");

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
        atlas_error("Failed to create GLFW window");
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

DeviceInfo Device::getDeviceInfo() {
    DeviceInfo info;
#ifdef OPENGL
    info.deviceName = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    info.vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    info.driverVersion = "N/A";
    info.renderingVersion = reinterpret_cast<const char *>(
        glGetString(GL_SHADING_LANGUAGE_VERSION));
    info.opalVersion = OPAL_VERSION;
    return info;
#else
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(this->physicalDevice, &deviceProperties);
    info.driverVersion = std::to_string(deviceProperties.driverVersion);
    info.deviceName = deviceProperties.deviceName;
    info.vendorName = std::to_string(deviceProperties.vendorID);
    info.renderingVersion = std::to_string(deviceProperties.apiVersion);
    info.opalVersion = OPAL_VERSION;
    return info;
#endif
}

std::shared_ptr<Device>
Device::acquire([[maybe_unused]] std::shared_ptr<Context> context) {
#ifdef OPENGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        atlas_error("Failed to initialize GLAD");
        throw std::runtime_error("Failed to initialize GLAD");
    }

    atlas_log("Graphics device acquired (OpenGL)");

    auto device = std::make_shared<Device>();

    Device::globalInstance = device.get();
    return device;
#else
    auto device = std::make_shared<Device>();
    Device::globalInstance = device.get();
    device->context = context;
    device->pickPhysicalDevice(context);
    device->createLogicalDevice(context);
    device->createSwapChain(context);
    device->createImageViews();
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

#ifdef VULKAN
VkDevice Device::globalDevice = VK_NULL_HANDLE;
#endif

#ifdef VULKAN
std::shared_ptr<Buffer> Device::getDefaultInstanceBuffer() {
    if (defaultInstanceBuffer != nullptr) {
        return defaultInstanceBuffer;
    }

    static const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                       0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f};

    defaultInstanceBuffer =
        Buffer::create(BufferUsage::VertexBuffer, sizeof(identity), identity,
                       MemoryUsageType::GPUOnly);
    return defaultInstanceBuffer;
}
#endif

} // namespace opal
