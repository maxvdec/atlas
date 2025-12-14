/*
 window.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/network/pipe.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "hydra/fluid.h"
#include "bezel/body.h"
#include "finewave/audio.h"
#include <atlas/window.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sys/resource.h>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <tuple>
#include <opal/opal.h>

Window *Window::mainWindow = nullptr;

Window::Window(WindowConfiguration config)
    : title(config.title), width(config.width), height(config.height) {
    atlas_log("Initializing window: " + config.title);
#ifdef VULKAN
    auto context = opal::Context::create({.useOpenGL = false});
    atlas_log("Using Vulkan backend");
#else
    auto context =
        opal::Context::create({.useOpenGL = true,
                               .majorVersion = 4,
                               .minorVersion = 1,
                               .profile = opal::OpenGLProfile::Core});
    atlas_log("Using OpenGL backend");
#endif

    context->setFlag(GLFW_DECORATED, config.decorations);
    context->setFlag(GLFW_RESIZABLE, config.resizable);
    context->setFlag(GLFW_TRANSPARENT_FRAMEBUFFER, config.transparent);
    context->setFlag(GLFW_FLOATING, config.alwaysOnTop);
    context->setFlag(GLFW_SAMPLES, config.multisampling ? 4 : 0);

#ifdef __APPLE__
    context->setFlag(GLFW_COCOA_RETINA_FRAMEBUFFER, true);
#endif

    GLFWwindow *window = context->makeWindow(
        config.width, config.height, config.title.c_str(), nullptr, nullptr);

    context->makeCurrent();

    device = opal::Device::acquire(context);

    glfwSetWindowOpacity(window, config.opacity);
    glfwSetInputMode(window, GLFW_CURSOR,
                     config.mouseCaptured ? GLFW_CURSOR_DISABLED
                                          : GLFW_CURSOR_NORMAL);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    device->getDefaultFramebuffer()->setViewport(0, 0, fbWidth, fbHeight);

    if (config.posX != WINDOW_CENTERED && config.posY != WINDOW_CENTERED) {
        glfwSetWindowPos(window, config.posX, config.posY);
    }

    if (config.aspectRatioX != DEFAULT_ASPECT_RATIO &&
        config.aspectRatioY != DEFAULT_ASPECT_RATIO) {
        glfwSetWindowAspectRatio(window, config.aspectRatioX,
                                 config.aspectRatioY);
    }

    this->windowRef = static_cast<CoreWindowReference>(window);

    this->renderScale = std::clamp(config.renderScale, 0.5f, 1.0f);
    this->ssaoRenderScale = std::clamp(config.ssaoScale, 0.25f, 1.0f);

    Window::mainWindow = this;

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int, int) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
        if (Window::mainWindow != nullptr) {
            Window::mainWindow->viewportWidth = fbWidth;
            Window::mainWindow->viewportHeight = fbHeight;
            Window::mainWindow->shadowMapsDirty = true;
            Window::mainWindow->ssaoMapsDirty = true;
        }
    });

    lastMouseX = width / 2.0;
    lastMouseY = height / 2.0;

    glfwSetCursorPosCallback(
        window, [](GLFWwindow *, double xpos, double ypos) {
            Window *window = Window::mainWindow;
            Position2d movement = {(float)xpos - window->lastMouseX,
                                   window->lastMouseY - (float)ypos};
            if (window->currentScene != nullptr) {
                window->currentScene->onMouseMove(*window, movement);
            }
            window->lastMouseX = (float)xpos;
            window->lastMouseY = (float)ypos;
        });

    glfwSetScrollCallback(
        window, [](GLFWwindow *, double xoffset, double yoffset) {
            Window *window = Window::mainWindow;
            Position2d offset = {(float)xoffset, (float)yoffset};
            if (window->currentScene != nullptr) {
                window->currentScene->onMouseScroll(*window, offset);
            }
        });

    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Depth);
    vertexShader.compile();
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Empty);
    fragmentShader.compile();
    ShaderProgram program = ShaderProgram();
    program.vertexShader = vertexShader;
    program.fragmentShader = fragmentShader;
    program.compile();
    this->depthProgram = program;

#ifdef __APPLE__
    this->useMultiPassPointShadows = true;
#else
    this->useMultiPassPointShadows = false;
#endif

    ShaderProgram pointProgram = ShaderProgram();
    if (this->useMultiPassPointShadows) {
        // Use multi-pass shaders without geometry shader
        VertexShader pointVertexShader = VertexShader::fromDefaultShader(
            AtlasVertexShader::PointLightShadowNoGeom);
        pointVertexShader.compile();
        FragmentShader pointFragmentShader = FragmentShader::fromDefaultShader(
            AtlasFragmentShader::PointLightShadowNoGeom);
        pointFragmentShader.compile();
        pointProgram.vertexShader = pointVertexShader;
        pointProgram.fragmentShader = pointFragmentShader;
        pointProgram.compile();
    } else {
        // Use single-pass shaders with geometry shader
        VertexShader pointVertexShader = VertexShader::fromDefaultShader(
            AtlasVertexShader::PointLightShadow);
        pointVertexShader.compile();
        FragmentShader pointFragmentShader = FragmentShader::fromDefaultShader(
            AtlasFragmentShader::PointLightShadow);
        pointFragmentShader.compile();
        GeometryShader geometryShader = GeometryShader::fromDefaultShader(
            AtlasGeometryShader::PointLightShadow);
        geometryShader.compile();
        pointProgram.vertexShader = pointVertexShader;
        pointProgram.fragmentShader = pointFragmentShader;
        pointProgram.geometryShader = geometryShader;
        pointProgram.compile();
    }
    this->pointDepthProgram = pointProgram;

    this->deferredProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Deferred, AtlasFragmentShader::Deferred);
    this->lightProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Light);
    this->bloomBlurProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Fullscreen, AtlasFragmentShader::GaussianBlur);
    this->volumetricProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Volumetric, AtlasFragmentShader::Volumetric);
    this->ssrProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSR);

    this->setupSSAO();

    audioEngine = std::make_shared<AudioEngine>();
    bool result = audioEngine->initialize();
    if (!result) {
        atlas_error("Failed to initialize audio engine");
        throw std::runtime_error("Failed to initialize audio engine");
    }
    atlas_log("Audio engine initialized successfully");

    opal::DeviceInfo info = device->getDeviceInfo();

    std::cout << "\033[1m\033[36mAtlas Engine\033[0m" << std::endl;
    std::cout << "\033[1m\033[36mVersion " << ATLAS_VERSION << " \033[0m"
              << std::endl;
    std::cout << "\033[1m\033[31mUsing Opal Graphics Library - Version "
              << info.opalVersion << " \033[0m" << std::endl;
#ifdef OPENGL
    std::cout << "\033[1m\033[32mUsing OpenGL Backend\033[0m" << std::endl;
#else
    std::cout << "\033[1m\033[32mUsing Vulkan Backend\033[0m" << std::endl;
    std::cout << "\033[1m\033[35m---------------\033[0m" << std::endl;
    std::cout << "\033[1m\033[35mUsing GPU: " << info.deviceName << "\033[0m"
              << std::endl;
    std::cout << "\033[1m\033[35mVendor ID: " << info.vendorName << "\033[0m"
              << std::endl;
    std::cout << "\033[1m\033[35mDriver Version: " << info.driverVersion
              << "\033[0m" << std::endl;
    std::cout << "\033[1m\033[35mAPI Version: " << info.renderingVersion
              << "\033[0m" << std::endl;
#endif

    TracerServices::getInstance().startTracing(TRACER_PORT);
    atlas_log("Atlas Tracer initialized.");
}

std::tuple<int, int> Window::getCursorPosition() {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return {static_cast<int>(xpos), static_cast<int>(ypos)};
}

void Window::run() {
    if (this->camera == nullptr) {
        this->camera = new Camera();
    }
    for (auto &obj : this->renderables) {
        obj->initialize();
    }
    for (auto &obj : this->preferenceRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->firstRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->lateForwardRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->uiRenderables) {
        obj->initialize();
    }
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);

    auto commandBuffer = device->acquireCommandBuffer();
    this->activeCommandBuffer = commandBuffer;

    this->lastTime = static_cast<float>(glfwGetTime());

    updatePipelineStateField(useMultisampling, true);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->useBlending, true);
    updatePipelineStateField(this->srcBlend, opal::BlendFunc::SrcAlpha);
    updatePipelineStateField(this->dstBlend, opal::BlendFunc::OneMinusSrcAlpha);

    this->framesPerSecond = 0.0f;

    auto defaultFramebuffer = device->getDefaultFramebuffer();
    auto renderPass = opal::RenderPass::create();
    renderPass->setFramebuffer(defaultFramebuffer);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        DebugTimer cpuTimer("Cpu Data");
        DebugTimer mainTimer("Main Loop");

        if (this->currentScene == nullptr) {
            commandBuffer->start();
            commandBuffer->beginPass(renderPass);
            commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                                      this->clearColor.b, this->clearColor.a);
            commandBuffer->clearDepth(1.0);
            commandBuffer->endPass();
            commandBuffer->commit();
#ifdef OPENGL
            glfwSwapBuffers(window);
#endif
            continue;
        }

        commandBuffer->start();
        float currentTime = static_cast<float>(glfwGetTime());
        this->deltaTime = currentTime - this->lastTime;
        lastTime = currentTime;

        if (this->deltaTime > 0.0f) {
            this->framesPerSecond = 1.0f / this->deltaTime;
        }

        currentScene->updateScene(this->deltaTime);

        // Update the renderables
        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            obj->update(*this);
        }

        for (auto &obj : this->lateForwardRenderables) {
            obj->update(*this);
        }

        currentScene->update(*this);

        uint64_t cpuTime = cpuTimer.stop();

        DebugTimer gpuTimer("Gpu Data");

        renderLightsToShadowMaps(commandBuffer);

        updatePipelineStateField(this->cullMode, opal::CullMode::None);
        updatePipelineStateField(this->cullMode, opal::CullMode::Back);
        // Render to the targets
        for (auto &target : this->renderTargets) {
            this->currentRenderTarget = target;
            updatePipelineStateField(this->depthCompareOp,
                                     opal::CompareOp::Less);
            updatePipelineStateField(this->writeDepth, true);
            updatePipelineStateField(this->cullMode, opal::CullMode::Back);

            auto renderPass = opal::RenderPass::create();
            renderPass->setFramebuffer(target->getFramebuffer());
            commandBuffer->beginPass(renderPass);
            if (target->brightTexture.id != 0) {
                target->getFramebuffer()->setDrawBuffers(2);
            }

            if (this->usesDeferred) {
                this->deferredRendering(target, commandBuffer);

                auto resolveCommand = opal::ResolveAction::create(
                    this->gBuffer->getFramebuffer(), target->getFramebuffer());
                commandBuffer->performResolve(resolveCommand);

                target->getFramebuffer()->bindForRead();
                target->getFramebuffer()->setDrawBuffers(2);

                updatePipelineStateField(this->useDepth, true);
                updatePipelineStateField(this->depthCompareOp,
                                         opal::CompareOp::Less);
                updatePipelineStateField(this->writeDepth, true);
                updatePipelineStateField(this->cullMode, opal::CullMode::Back);

                for (auto &obj : this->firstRenderables) {
                    obj->setViewMatrix(this->camera->calculateViewMatrix());
                    obj->setProjectionMatrix(calculateProjectionMatrix());
                    obj->render(getDeltaTime(), commandBuffer,
                                shouldRefreshPipeline(obj));
                }

                for (auto &obj : this->renderables) {
                    if (obj->renderLateForward) {
                        continue;
                    }
                    if (!obj->canUseDeferredRendering()) {
                        obj->setViewMatrix(this->camera->calculateViewMatrix());
                        obj->setProjectionMatrix(calculateProjectionMatrix());
                        obj->render(getDeltaTime(), commandBuffer,
                                    shouldRefreshPipeline(obj));
                    }
                }

                for (auto &obj : this->lateForwardRenderables) {
                    obj->setViewMatrix(this->camera->calculateViewMatrix());
                    obj->setProjectionMatrix(calculateProjectionMatrix());
                    obj->render(getDeltaTime(), commandBuffer,
                                shouldRefreshPipeline(obj));
                }

                commandBuffer->endPass();
                target->resolve();
                continue;
            }
            commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                                      this->clearColor.b, this->clearColor.a);
            commandBuffer->clearDepth(1.0f);

            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }

            for (auto &obj : this->renderables) {
                if (obj->renderLateForward) {
                    continue;
                }
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }
            updateFluidCaptures(commandBuffer);
            for (auto &obj : this->lateForwardRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }
            target->resolve();

            commandBuffer->endPass();
        }

        // Render to the screen
        commandBuffer->beginPass(renderPass);
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        setViewportState(0, 0, fbWidth, fbHeight);
        commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                                  this->clearColor.b, this->clearColor.a);
        commandBuffer->clearDepth(1.0f);

        if (this->renderTargets.empty()) {
            updateBackbufferTarget(fbWidth, fbHeight);
            this->currentRenderTarget = this->screenRenderTarget.get();
            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }

            for (auto &obj : this->renderables) {
                if (obj->renderLateForward) {
                    continue;
                }
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }

            updateFluidCaptures(commandBuffer);

            for (auto &obj : this->lateForwardRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }
        } else {
            this->currentRenderTarget = nullptr;
        }

        updatePipelineStateField(this->cullMode, opal::CullMode::None);
        for (auto &obj : this->preferenceRenderables) {
            RenderTarget *target = dynamic_cast<RenderTarget *>(obj);
            if (target != nullptr && target->brightTexture.id != 0) {
                this->renderPhysicalBloom(target);
            }
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }

        updatePipelineStateField(this->cullMode, opal::CullMode::Back);
        updatePipelineStateField(this->useBlending, true);

        for (auto &obj : this->uiRenderables) {
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }

        this->lastViewMatrix = this->camera->calculateViewMatrix();

        commandBuffer->endPass();
        commandBuffer->commit();
#ifdef OPENGL
        glfwSwapBuffers(window);
#endif

        uint64_t gpuTime = gpuTimer.stop();
        uint64_t mainTime = mainTimer.stop();

        FrameDrawInfo frameInfo{};
        frameInfo.drawCallCount = commandBuffer->getAndResetDrawCallCount();
        frameInfo.frameTimeMs = this->deltaTime * 1000.0f;
        frameInfo.frameNumber = device->frameCount;
        frameInfo.fps = this->framesPerSecond;
        frameInfo.send();

        FrameResourcesInfo frameResourcesInfo{};
        frameResourcesInfo.frameNumber = device->frameCount;
        frameResourcesInfo.resourcesCreated =
            ResourceTracker::getInstance().createdResources;
        frameResourcesInfo.resourcesUnloaded =
            ResourceTracker::getInstance().unloadedResources;
        frameResourcesInfo.resourcesLoaded =
            ResourceTracker::getInstance().loadedResources;
        frameResourcesInfo.totalMemoryMb =
            ResourceTracker::getInstance().totalMemoryMb;

        FrameMemoryPacket memoryPacket{};
        memoryPacket.frameNumber = device->frameCount;
        memoryPacket.allocationCount =
            ResourceTracker::getInstance().createdResources -
            ResourceTracker::getInstance().unloadedResources;
        memoryPacket.totalAllocatedMb =
            ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.totalCPUMb = ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.totalGPUMb = ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.deallocationCount =
            ResourceTracker::getInstance().unloadedResources;
        memoryPacket.send();

        rusage usage{};
        getrusage(RUSAGE_SELF, &usage);

        double normalCpuTime =
            usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1e6) +
            usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1e6);

        TimingEventPacket timingEvent{};
        timingEvent.frameNumber = device->frameCount;
        timingEvent.durationMs = static_cast<float>(gpuTime) / 1'000'000.0f;
        timingEvent.name = "Main Loop";
        timingEvent.subsystem = TimingEventSubsystem::Rendering;
        timingEvent.send();

        FrameTimingPacket timingPacket{};
        timingPacket.frameNumber = device->frameCount;
        timingPacket.cpuFrameTimeMs =
            static_cast<float>(cpuTime) / 1'000'000.0f;
        timingPacket.gpuFrameTimeMs =
            static_cast<float>(gpuTime) / 1'000'000.0f;
        timingPacket.workerThreadTimeMs = 0.0f;
        timingPacket.mainThreadTimeMs =
            static_cast<float>(mainTime) / 1'000'000.0f;
        timingPacket.memoryMb = ResourceTracker::getInstance().totalMemoryMb;
        timingPacket.cpuUsagePercent =
            static_cast<float>(normalCpuTime / this->deltaTime * 100.0);
        timingPacket.gpuUsagePercent = 0.0f;
        timingPacket.send();

        ResourceTracker::getInstance().createdResources = 0;
        ResourceTracker::getInstance().loadedResources = 0;
        ResourceTracker::getInstance().unloadedResources = 0;
        ResourceTracker::getInstance().totalMemoryMb = 0.0f;
        frameResourcesInfo.send();
    }
}

void Window::addObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }

    this->renderables.push_back(obj);

    if (obj->renderLateForward) {
        this->addLateForwardObject(obj);
    }
}

void Window::addLateForwardObject(Renderable *object) {
    if (object == nullptr) {
        return;
    }

    if (std::find(lateForwardRenderables.begin(), lateForwardRenderables.end(),
                  object) == lateForwardRenderables.end()) {
        lateForwardRenderables.push_back(object);
    }

    if (auto *fluid = dynamic_cast<Fluid *>(object)) {
        if (std::find(lateFluids.begin(), lateFluids.end(), fluid) ==
            lateFluids.end()) {
            lateFluids.push_back(fluid);
        }
    }
}

void Window::addPreferencedObject(Renderable *obj) {
    this->preferenceRenderables.push_back(obj);
}

void Window::close() {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    glfwSetWindowShouldClose(window, true);
}

void Window::setCamera(Camera *newCamera) { this->camera = newCamera; }

void Window::setScene(Scene *scene) {
    atlas_log("Setting active scene");
    this->currentScene = scene;
    scene->initialize(*this);
    this->shadowMapsDirty = true;
    this->shadowUpdateCooldown = 0.0f;
    this->lastShadowCameraPosition.reset();
    this->lastShadowCameraDirection.reset();
    this->cachedDirectionalLightDirections.clear();
    this->cachedPointLightPositions.clear();
    this->cachedSpotlightPositions.clear();
    this->cachedSpotlightDirections.clear();
    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
    this->lastSSAOCameraPosition.reset();
    this->lastSSAOCameraDirection.reset();
}

glm::mat4 Window::calculateProjectionMatrix() {
    glm::mat4 projection{1.0f};
    if (!this->camera->useOrthographic) {
        int fbWidth, fbHeight;
        GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        projection = glm::perspective(glm::radians(camera->fov), aspectRatio,
                                      camera->nearClip, camera->farClip);
    } else {
        float orthoSize = this->camera->orthographicSize;
        int fbWidth, fbHeight;
        GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        projection = glm::ortho(-orthoSize * aspectRatio,
                                orthoSize * aspectRatio, -orthoSize, orthoSize,
                                camera->nearClip, camera->farClip);
    }

    // For Vulkan, flip Y in projection to match GL-style coordinates
#ifdef VULKAN
    projection[1][1] *= -1.0f;
#endif
    return projection;
}

void Window::setFullscreen(bool enable) {
    atlas_log(enable ? "Switching to fullscreen mode"
                     : "Switching to windowed mode");
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    if (enable) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    } else {
        int width = this->width;
        int height = this->height;
        glfwSetWindowMonitor(window, nullptr, 100, 100, width, height, 0);
    }
}

void Window::setFullscreen(Monitor &monitor) {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(monitor.monitorRef);
    const GLFWvidmode *mode = glfwGetVideoMode(glfwMonitor);
    glfwSetWindowMonitor(window, glfwMonitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);
}

void Window::setWindowed(WindowConfiguration config) {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int width = config.width;
    int height = config.height;
    this->renderScale = std::clamp(config.renderScale, 0.5f, 1.0f);
    this->ssaoRenderScale = std::clamp(config.ssaoScale, 0.25f, 1.0f);
    int posX = config.posX != WINDOW_CENTERED ? config.posX : 100;
    int posY = config.posY != WINDOW_CENTERED ? config.posY : 100;
    glfwSetWindowMonitor(window, nullptr, posX, posY, width, height, 0);
    this->shadowMapsDirty = true;
    this->ssaoMapsDirty = true;
}

std::vector<Monitor> Window::enumerateMonitors() {
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    std::vector<Monitor> monitorList;
    for (int i = 0; i < count; ++i) {
        bool isPrimary = (monitors[i] == glfwGetPrimaryMonitor());
        monitorList.emplace_back(static_cast<CoreMonitorReference>(monitors[i]),
                                 i, isPrimary);
    }
    return monitorList;
}

Window::~Window() {
    // Release opal framebuffers and textures (shared_ptr handles cleanup)
    this->pingpongFramebuffers[0] = nullptr;
    this->pingpongFramebuffers[1] = nullptr;
    this->pingpongTextures[0] = nullptr;
    this->pingpongTextures[1] = nullptr;
    this->pingpongWidth = 0;
    this->pingpongHeight = 0;

    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    glfwDestroyWindow(window);
    glfwTerminate();
}

Monitor::Monitor(CoreMonitorReference ref, int id, bool isPrimary)
    : monitorID(id), primary(isPrimary), monitorRef(ref) {}

std::vector<VideoMode> Monitor::queryVideoModes() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    int count;
    const GLFWvidmode *modes = glfwGetVideoModes(glfwMonitor, &count);
    std::vector<VideoMode> videoModes;
    for (int i = 0; i < count; ++i) {
        videoModes.push_back(
            {modes[i].width, modes[i].height, modes[i].refreshRate});
    }
    return videoModes;
}

VideoMode Monitor::getCurrentVideoMode() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    const GLFWvidmode *mode = glfwGetVideoMode(glfwMonitor);
    return {mode->width, mode->height, mode->refreshRate};
}

std::tuple<int, int> Monitor::getPhysicalSize() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    int widthMM, heightMM;
    glfwGetMonitorPhysicalSize(glfwMonitor, &widthMM, &heightMM);
    return {widthMM, heightMM};
}

std::tuple<int, int> Monitor::getPosition() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    int posX, posY;
    glfwGetMonitorPos(glfwMonitor, &posX, &posY);
    return {posX, posY};
}

std::tuple<float, float> Monitor::getContentScale() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    float scaleX, scaleY;
    glfwGetMonitorContentScale(glfwMonitor, &scaleX, &scaleY);
    return {scaleX, scaleY};
}

std::string Monitor::getName() {
    GLFWmonitor *glfwMonitor = static_cast<GLFWmonitor *>(this->monitorRef);
    return std::string(glfwGetMonitorName(glfwMonitor));
}

float Window::getTime() { return static_cast<float>(glfwGetTime()); }

bool Window::isKeyPressed(Key key) {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Window::isKeyClicked(Key key) {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS;
}

void Window::releaseMouse() {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::captureMouse() {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::addRenderTarget(RenderTarget *target) {
    this->renderTargets.push_back(target);
}

void Window::renderLightsToShadowMaps(
    std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (this->currentScene == nullptr) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    this->shadowUpdateCooldown =
        std::max(0.0f, this->shadowUpdateCooldown - this->deltaTime);

    bool cameraMoved = false;
    if (this->camera != nullptr) {
        glm::vec3 currentPos = this->camera->position.toGlm();
        glm::vec3 currentDir = this->camera->getFrontVector().toGlm();
        if (!this->lastShadowCameraPosition.has_value() ||
            !this->lastShadowCameraDirection.has_value()) {
            cameraMoved = true;
        } else {
            glm::vec3 lastPos = this->lastShadowCameraPosition->toGlm();
            glm::vec3 lastDir = this->lastShadowCameraDirection->toGlm();
            if (glm::length(currentPos - lastPos) > 0.25f) {
                cameraMoved = true;
            } else if (glm::length(currentDir - lastDir) > 0.02f) {
                cameraMoved = true;
            }
        }
    }

    bool lightsChanged = false;
    const float positionThreshold = 0.1f;
    const float directionThreshold = 0.02f;

    const auto &directionalLights = this->currentScene->directionalLights;
    if (this->cachedDirectionalLightDirections.size() !=
        directionalLights.size()) {
        lightsChanged = true;
    } else {
        for (size_t i = 0; i < directionalLights.size(); ++i) {
            if (directionalLights[i] == nullptr) {
                if (glm::length(this->cachedDirectionalLightDirections[i]) >
                    directionThreshold) {
                    lightsChanged = true;
                    break;
                }
                continue;
            }
            glm::vec3 dir = directionalLights[i]->direction.toGlm();
            if (glm::length(dir - this->cachedDirectionalLightDirections[i]) >
                directionThreshold) {
                lightsChanged = true;
                break;
            }
        }
    }

    const auto &pointLights = this->currentScene->pointLights;
    if (!lightsChanged) {
        if (this->cachedPointLightPositions.size() != pointLights.size()) {
            lightsChanged = true;
        } else {
            for (size_t i = 0; i < pointLights.size(); ++i) {
                if (pointLights[i] == nullptr) {
                    if (glm::length(this->cachedPointLightPositions[i]) >
                        positionThreshold) {
                        lightsChanged = true;
                        break;
                    }
                    continue;
                }
                glm::vec3 pos = pointLights[i]->position.toGlm();
                if (glm::length(pos - this->cachedPointLightPositions[i]) >
                    positionThreshold) {
                    lightsChanged = true;
                    break;
                }
            }
        }
    }

    const auto &spotLights = this->currentScene->spotlights;
    if (!lightsChanged) {
        if (this->cachedSpotlightPositions.size() != spotLights.size() ||
            this->cachedSpotlightDirections.size() != spotLights.size()) {
            lightsChanged = true;
        } else {
            for (size_t i = 0; i < spotLights.size(); ++i) {
                if (spotLights[i] == nullptr) {
                    bool cachedPosNonZero =
                        glm::length(this->cachedSpotlightPositions[i]) >
                        positionThreshold;
                    bool cachedDirNonZero =
                        glm::length(this->cachedSpotlightDirections[i]) >
                        directionThreshold;
                    if (cachedPosNonZero || cachedDirNonZero) {
                        lightsChanged = true;
                        break;
                    }
                    continue;
                }
                glm::vec3 pos = spotLights[i]->position.toGlm();
                glm::vec3 dir = spotLights[i]->direction.toGlm();
                if (glm::length(pos - this->cachedSpotlightPositions[i]) >
                        positionThreshold ||
                    glm::length(dir - this->cachedSpotlightDirections[i]) >
                        directionThreshold) {
                    lightsChanged = true;
                    break;
                }
            }
        }
    }

    if (cameraMoved || lightsChanged) {
        this->shadowMapsDirty = true;
    }

    if (!this->shadowMapsDirty) {
        return;
    }

    if (this->shadowUpdateCooldown > 0.0f) {
        return;
    }

    this->shadowMapsDirty = false;
    this->shadowUpdateCooldown = this->shadowUpdateInterval;

    bool renderedShadows = false;

    std::vector<std::shared_ptr<opal::Pipeline>> originalPipelines;
    for (auto &obj : this->renderables) {
        if (obj->getPipeline() != std::nullopt) {
            originalPipelines.push_back(obj->getPipeline().value());
        } else {
            originalPipelines.push_back(opal::Pipeline::create());
        }
    }

    std::vector<std::shared_ptr<opal::Pipeline>> originalLatePipelines;
    for (auto &obj : this->lateForwardRenderables) {
        if (obj->getPipeline() != std::nullopt) {
            originalLatePipelines.push_back(obj->getPipeline().value());
        } else {
            originalLatePipelines.push_back(opal::Pipeline::create());
        }
    }

    auto gatherShadowCasters = [this]() {
        std::vector<Renderable *> casters;
        casters.reserve(this->renderables.size() +
                        this->lateForwardRenderables.size());
        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            casters.push_back(obj);
        }
        for (auto &obj : this->lateForwardRenderables) {
            casters.push_back(obj);
        }
        return casters;
    };

    std::shared_ptr<opal::Pipeline> depthPipeline = opal::Pipeline::create();

    for (auto &light : this->currentScene->directionalLights) {
        if (light->doesCastShadows == false) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;

        depthPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        depthPipeline->setCullMode(opal::CullMode::Back);
        depthPipeline->enablePolygonOffset(true);
        depthPipeline->setPolygonOffset(2.0f, 4.0f);

        depthPipeline = this->depthProgram.requestPipeline(depthPipeline);

        shadowRenderTarget->bind();
        commandBuffer->clearDepth(1.0f);
        ShadowParams lightParams =
            light->calculateLightSpaceMatrix(gatherShadowCasters());
        glm::mat4 lightView = lightParams.lightView;
        glm::mat4 lightProjection = lightParams.lightProjection;
        light->lastShadowParams = lightParams;
        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }

        for (auto &obj : this->lateForwardRenderables) {
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);
            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }
    }

    std::shared_ptr<opal::Pipeline> spotlightsPipeline =
        opal::Pipeline::create();

    for (auto &light : this->currentScene->spotlights) {
        if (light->doesCastShadows == false) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        spotlightsPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        spotlightsPipeline->setCullMode(opal::CullMode::Back);
        spotlightsPipeline->enablePolygonOffset(true);
        spotlightsPipeline->setPolygonOffset(2.0f, 4.0f);
        spotlightsPipeline =
            this->depthProgram.requestPipeline(spotlightsPipeline);

        shadowRenderTarget->bind();
        commandBuffer->clearDepth(1.0f);
        std::tuple<glm::mat4, glm::mat4> lightSpace =
            light->calculateLightSpaceMatrix();
        glm::mat4 lightView = std::get<0>(lightSpace);
        glm::mat4 lightProjection = std::get<1>(lightSpace);
        ShadowParams cached;
        cached.lightView = lightView;
        cached.lightProjection = lightProjection;
        cached.bias = 0.005f;
        light->lastShadowParams = cached;
        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }

        for (auto &obj : this->lateForwardRenderables) {
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);
            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }
    }

    std::shared_ptr<opal::Pipeline> pointLightPipeline =
        opal::Pipeline::create();

    for (auto &light : this->currentScene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        pointLightPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        pointLightPipeline->setCullMode(opal::CullMode::Back);
        pointLightPipeline->enablePolygonOffset(true);
        pointLightPipeline->setPolygonOffset(2.0f, 4.0f);
        pointLightPipeline =
            this->pointDepthProgram.requestPipeline(pointLightPipeline);

        std::vector<glm::mat4> shadowTransforms =
            light->calculateShadowTransforms();

        pointLightPipeline->setUniform3f("lightPos", light->position.x,
                                         light->position.y, light->position.z);
        pointLightPipeline->setUniform1f("far_plane", light->distance);
        light->lastShadowParams.farPlane = light->distance;

        if (this->useMultiPassPointShadows) {
            // Multi-pass rendering: render 6 times, once per cubemap face
            for (int face = 0; face < 6; ++face) {
                shadowRenderTarget->bindCubemapFace(face);
                commandBuffer->clearDepth(1.0f);

                // Set the shadow matrix for this face
                pointLightPipeline->setUniformMat4f("shadowMatrix",
                                                    shadowTransforms[face]);
                pointLightPipeline->setUniform1i("faceIndex", face);

                for (auto &obj : this->renderables) {
                    if (obj->renderLateForward) {
                        continue;
                    }
                    if (obj->getPipeline() == std::nullopt ||
                        !obj->canCastShadows()) {
                        continue;
                    }

                    obj->setProjectionMatrix(glm::mat4(1.0));
                    obj->setViewMatrix(glm::mat4(1.0));
                    obj->setPipeline(pointLightPipeline);
                    obj->render(getDeltaTime(), commandBuffer, false);
                }

                for (auto &obj : this->lateForwardRenderables) {
                    if (obj->getPipeline() == std::nullopt ||
                        !obj->canCastShadows()) {
                        continue;
                    }

                    obj->setProjectionMatrix(glm::mat4(1.0));
                    obj->setViewMatrix(glm::mat4(1.0));
                    obj->setPipeline(pointLightPipeline);
                    obj->render(getDeltaTime(), commandBuffer, false);
                }
            }
        } else {
            // Single-pass rendering with geometry shader
            shadowRenderTarget->bind();
            commandBuffer->clearDepth(1.0f);

            for (size_t i = 0; i < shadowTransforms.size(); ++i) {
                pointLightPipeline->setUniformMat4f("shadowMatrices[" +
                                                        std::to_string(i) + "]",
                                                    shadowTransforms[i]);
            }

            for (auto &obj : this->renderables) {
                if (obj->renderLateForward) {
                    continue;
                }
                if (obj->getPipeline() == std::nullopt ||
                    !obj->canCastShadows()) {
                    continue;
                }

                obj->setProjectionMatrix(glm::mat4(1.0));
                obj->setViewMatrix(glm::mat4(1.0));
                obj->setPipeline(pointLightPipeline);
                obj->render(getDeltaTime(), commandBuffer, false);
            }

            for (auto &obj : this->lateForwardRenderables) {
                if (obj->getPipeline() == std::nullopt ||
                    !obj->canCastShadows()) {
                    continue;
                }

                obj->setProjectionMatrix(glm::mat4(1.0));
                obj->setViewMatrix(glm::mat4(1.0));
                obj->setPipeline(pointLightPipeline);
                obj->render(getDeltaTime(), commandBuffer, false);
            }
        }
    }

    if (this->camera != nullptr) {
        this->lastShadowCameraPosition = this->camera->position;
        this->lastShadowCameraDirection = this->camera->getFrontVector();
    }

    this->cachedDirectionalLightDirections.clear();
    this->cachedDirectionalLightDirections.reserve(directionalLights.size());
    for (auto *light : directionalLights) {
        if (light == nullptr) {
            this->cachedDirectionalLightDirections.emplace_back(0.0f, 0.0f,
                                                                0.0f);
            continue;
        }
        this->cachedDirectionalLightDirections.push_back(
            light->direction.toGlm());
    }

    this->cachedPointLightPositions.clear();
    this->cachedPointLightPositions.reserve(pointLights.size());
    for (auto *light : pointLights) {
        if (light == nullptr) {
            this->cachedPointLightPositions.emplace_back(0.0f, 0.0f, 0.0f);
            continue;
        }
        this->cachedPointLightPositions.push_back(light->position.toGlm());
    }

    this->cachedSpotlightPositions.clear();
    this->cachedSpotlightDirections.clear();
    this->cachedSpotlightPositions.reserve(spotLights.size());
    this->cachedSpotlightDirections.reserve(spotLights.size());
    for (auto *light : spotLights) {
        if (light == nullptr) {
            this->cachedSpotlightPositions.emplace_back(0.0f, 0.0f, 0.0f);
            this->cachedSpotlightDirections.emplace_back(0.0f, 0.0f, 0.0f);
            continue;
        }
        this->cachedSpotlightPositions.push_back(light->position.toGlm());
        this->cachedSpotlightDirections.push_back(light->direction.toGlm());
    }

    // Polygon offset is controlled per-pipeline, no need to disable globally
    if (!renderedShadows) {
        return;
    }

    size_t i = 0;
    for (auto &renderable : this->renderables) {
        if (i < originalPipelines.size()) {
            renderable->setPipeline(originalPipelines[i]);
            i++;
        }
    }

    size_t j = 0;
    for (auto &renderable : this->lateForwardRenderables) {
        if (j < originalLatePipelines.size()) {
            renderable->setPipeline(originalLatePipelines[j]);
            j++;
        }
    }
}

std::vector<std::shared_ptr<Body>> Window::getAllBodies() {
    std::vector<std::shared_ptr<Body>> bodies;
    for (auto &obj : this->renderables) {
        CoreObject *coreObj = dynamic_cast<CoreObject *>(obj);
        if (coreObj != nullptr && coreObj->hasPhysics) {
            bodies.push_back(coreObj->body);
        }
    }
    return bodies;
}

void Window::renderPingpong(RenderTarget *target) {
    if (target->brightTexture.id == 0) {
        return;
    }

    if (!target->object || !target->object->isVisible) {
        return;
    }

    const int blurDownscaleFactor = 2;
    int blurWidth = std::max(1, target->brightTexture.creationData.width /
                                    blurDownscaleFactor);
    int blurHeight = std::max(1, target->brightTexture.creationData.height /
                                     blurDownscaleFactor);

    if (this->pingpongFramebuffers[0] == nullptr ||
        this->pingpongFramebuffers[1] == nullptr ||
        blurWidth != this->pingpongWidth ||
        blurHeight != this->pingpongHeight) {
        this->pingpongFramebuffers[0] = nullptr;
        this->pingpongFramebuffers[1] = nullptr;
        this->pingpongTextures[0] = nullptr;
        this->pingpongTextures[1] = nullptr;

        this->pingpongWidth = blurWidth;
        this->pingpongHeight = blurHeight;

        for (unsigned int i = 0; i < 2; i++) {
            auto texture = opal::Texture::create(
                opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F,
                blurWidth, blurHeight, opal::TextureDataFormat::Rgba, nullptr,
                1);
            texture->setFilterMode(opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);
            texture->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
            texture->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

            auto framebuffer = opal::Framebuffer::create(blurWidth, blurHeight);
            opal::Attachment colorAttachment;
            colorAttachment.texture = texture;
            colorAttachment.type = opal::Attachment::Type::Color;
            framebuffer->addAttachment(colorAttachment);

            if (!framebuffer->getStatus()) {
                std::cerr << "Pingpong Framebuffer not complete!" << std::endl;
            }

            this->pingpongFramebuffers[i] = framebuffer;
            this->pingpongTextures[i] = texture;
        }
    }

    device->getDefaultFramebuffer()->bind();
    device->frameCount++;

    bool horizontal = true;
    bool firstIteration = true;
    const unsigned int blurIterations = std::max(1u, this->bloomBlurPasses);

    auto originalPipeline = target->object->getPipeline();
    if (!originalPipeline.has_value()) {
        return;
    }

    auto targetProgram = originalPipeline.value();

    auto blurPipeline = opal::Pipeline::create();

    blurPipeline->setViewport(0, 0, blurWidth, blurHeight);
    blurPipeline->enableDepthTest(false);
    blurPipeline->enableBlending(false);

    blurPipeline = this->bloomBlurProgram.requestPipeline(blurPipeline);

    target->setPipeline(blurPipeline);

    blurPipeline->bind();
    blurPipeline->setUniform1f("radius", 2.5f);
    blurPipeline->setUniform1i("image", 0);

    target->object->vao->bind();
    target->object->ebo->bind();

    for (unsigned int i = 0; i < blurIterations; ++i) {
        this->pingpongFramebuffers[horizontal]->bind();
        activeCommandBuffer->clearColor(0.0f, 0.0f, 0.0f, 1.0f);

        blurPipeline->setUniform1i("horizontal", horizontal ? 1 : 0);

        blurPipeline->bindTexture2D(
            "image",
            firstIteration ? target->brightTexture.id
                           : this->pingpongTextures[!horizontal]->textureID,
            0);

        if (!target->object->indices.empty()) {
            activeCommandBuffer->drawIndexed(
                static_cast<uint>(target->object->indices.size()));
        } else {
            activeCommandBuffer->draw(
                static_cast<uint>(target->object->vertices.size()));
        }

        horizontal = !horizontal;
        firstIteration = false;
    }

    target->object->vao->unbind();

    target->object->setPipeline(targetProgram);
    device->getDefaultFramebuffer()->bind();

    updatePipelineStateField(this->useDepth, true);

    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    setViewportState(0, 0, fbWidth, fbHeight);

    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width = this->pingpongWidth;
    target->blurredTexture.creationData.height = this->pingpongHeight;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id = this->pingpongTextures[!horizontal]->textureID;
    target->blurredTexture.texture = this->pingpongTextures[!horizontal];
}

void Window::useDeferredRendering() {
    atlas_log("Enabling deferred rendering");
    this->usesDeferred = true;
    auto target = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::GBuffer));
    this->gBuffer = target;
    auto volumetricTarget = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::Scene));
    this->volumetricBuffer = volumetricTarget;
    auto ssrTarget = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::Scene));
    this->ssrFramebuffer = ssrTarget;
    this->ssaoMapsDirty = true;
}

void Window::renderPhysicalBloom(RenderTarget *target) {
    if (target->brightTexture.id == 0) {
        return;
    }

    if (this->bloomBuffer == nullptr) {
        this->bloomBuffer =
            std::make_shared<BloomRenderTarget>(BloomRenderTarget());
        int sizeX, sizeY;
        glfwGetFramebufferSize((GLFWwindow *)this->windowRef, &sizeX, &sizeY);
        this->bloomBuffer->init(
            sizeX, sizeY, currentScene->environment.lightBloom.maxSamples);
    }

    this->bloomBuffer->renderBloomTexture(
        target->brightTexture.id, currentScene->environment.lightBloom.radius);
    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width =
        this->bloomBuffer->srcViewportSizef.x;
    target->blurredTexture.creationData.height =
        this->bloomBuffer->srcViewportSizef.y;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id = this->bloomBuffer->getBloomTexture();
}

void Window::updateFluidCaptures(
    std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }
    for (auto *fluid : lateFluids) {
        if (fluid == nullptr) {
            continue;
        }
        if (fluid->captureDirty) {
            fluid->updateCapture(*this, commandBuffer);
        }
    }
}

void Window::captureFluidReflection(
    Fluid &fluid, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (!fluid.reflectionTarget) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    RenderTarget &target = *fluid.reflectionTarget;

    Camera *originalCamera = this->camera;
    Camera reflectionCamera = *originalCamera;

    glm::vec3 planePoint = fluid.calculatePlanePoint();
    glm::vec3 planeNormal = fluid.calculatePlaneNormal();

    glm::vec3 cameraPos = originalCamera->position.toGlm();
    float distance = glm::dot(planeNormal, cameraPos - planePoint);
    glm::vec3 reflectedPos = cameraPos - 2.0f * distance * planeNormal;

    glm::vec3 front = originalCamera->getFrontVector().toGlm();
    glm::vec3 reflectedDir =
        front - 2.0f * glm::dot(front, planeNormal) * planeNormal;
    glm::vec3 reflectedTarget = reflectedPos + reflectedDir;

    reflectionCamera.setPosition(Position3d::fromGlm(reflectedPos));
    reflectionCamera.lookAt(Position3d::fromGlm(reflectedTarget));
    reflectionCamera.fov = originalCamera->fov * 1.2f;
    reflectionCamera.nearClip = originalCamera->nearClip;
    reflectionCamera.farClip = originalCamera->farClip;
    reflectionCamera.useOrthographic = originalCamera->useOrthographic;
    reflectionCamera.orthographicSize = originalCamera->orthographicSize;

    Camera *cameraBackup = this->camera;
    this->camera = &reflectionCamera;

    glm::vec4 plane = fluid.calculateClipPlane();
    bool clipBackup = clipPlaneEnabled;
    glm::vec4 clipEquationBackup = clipPlaneEquation;
    clipPlaneEnabled = true;
    clipPlaneEquation = plane;

    RenderTarget *previousTarget = currentRenderTarget;
    const int previousViewportX = this->viewportX;
    const int previousViewportY = this->viewportY;
    const int previousViewportWidth = this->viewportWidth;
    const int previousViewportHeight = this->viewportHeight;

    target.bind();
    auto pipeline = opal::Pipeline::create();
    pipeline->enableClipDistance(0, true);

    target.getFramebuffer()->setViewport(0, 0, target.getWidth(),
                                         target.getHeight());
    setViewportState(0, 0, target.getWidth(), target.getHeight());
    target.getFramebuffer()->setDrawBuffers(2);

    const bool previousUseDepth = this->useDepth;
    const bool previousWriteDepth = this->writeDepth;
    const bool previousUseBlending = this->useBlending;
    const opal::CullMode previousCullMode = this->cullMode;
    const opal::CompareOp previousDepthCompare = this->depthCompareOp;

    pipeline->enableBlending(false);
    pipeline->enableDepthTest(true);
    pipeline->enableDepthWrite(true);
    pipeline->setCullMode(opal::CullMode::Front);
    pipeline->setDepthCompareOp(opal::CompareOp::Less);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, false);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->writeDepth, true);
    updatePipelineStateField(this->cullMode, opal::CullMode::Front);
    updatePipelineStateField(this->depthCompareOp, opal::CompareOp::Less);

    commandBuffer->clear(fluid.color.r, fluid.color.g, fluid.color.b, 1.0f,
                         1.0f);

    currentRenderTarget = &target;

    glm::mat4 view = this->camera->calculateViewMatrix();
    glm::mat4 projection = calculateProjectionMatrix();

    auto renderQueue = [&](const std::vector<Renderable *> &queue,
                           bool skipLate) {
        ShaderProgram oldProgram;
        for (auto *obj : queue) {
            if (obj == nullptr) {
                continue;
            }
            if (skipLate && obj->renderLateForward) {
                continue;
            }
            if (dynamic_cast<Fluid *>(obj) == &fluid) {
                continue;
            }
            if (obj->canUseDeferredRendering()) {
                oldProgram = obj->getShaderProgram().value();
                obj->setShader(ShaderProgram::fromDefaultShaders(
                    AtlasVertexShader::Main, AtlasFragmentShader::Main));
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));

            if (obj->canUseDeferredRendering()) {
                obj->setShader(oldProgram);
            }
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    if (previousTarget && previousTarget->getFramebuffer()) {
        previousTarget->bind();
        previousTarget->getFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    } else {
        device->getDefaultFramebuffer()->bind();
        device->getDefaultFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    }
    setViewportState(previousViewportX, previousViewportY,
                     previousViewportWidth, previousViewportHeight);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    pipeline->enableClipDistance(0, clipBackup);

    pipeline->enableBlending(previousUseBlending);
    pipeline->enableDepthTest(previousUseDepth);
    pipeline->enableDepthWrite(previousWriteDepth);
    pipeline->setCullMode(previousCullMode);
    pipeline->setDepthCompareOp(previousDepthCompare);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, previousUseBlending);
    updatePipelineStateField(this->useDepth, previousUseDepth);
    updatePipelineStateField(this->writeDepth, previousWriteDepth);
    updatePipelineStateField(this->cullMode, previousCullMode);
    updatePipelineStateField(this->depthCompareOp, previousDepthCompare);

    this->camera = cameraBackup;
}

void Window::captureFluidRefraction(
    Fluid &fluid, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (!fluid.refractionTarget) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    RenderTarget &target = *fluid.refractionTarget;

    glm::vec3 planePoint = fluid.calculatePlanePoint();
    glm::vec3 planeNormal = fluid.calculatePlaneNormal();

    const float clipBias = 0.02f;
    glm::vec4 plane =
        glm::vec4(planeNormal, -glm::dot(planeNormal, planePoint) - clipBias);

    bool clipBackup = clipPlaneEnabled;
    glm::vec4 clipEquationBackup = clipPlaneEquation;
    clipPlaneEnabled = true;
    clipPlaneEquation = plane;

    RenderTarget *previousTarget = currentRenderTarget;
    const int previousViewportX = this->viewportX;
    const int previousViewportY = this->viewportY;
    const int previousViewportWidth = this->viewportWidth;
    const int previousViewportHeight = this->viewportHeight;

    target.bind();
    auto pipeline = opal::Pipeline::create();
    pipeline->enableClipDistance(0, true);

    target.getFramebuffer()->setViewport(0, 0, target.getWidth(),
                                         target.getHeight());
    setViewportState(0, 0, target.getWidth(), target.getHeight());
    target.getFramebuffer()->setDrawBuffers(2);

    const bool previousUseDepth = this->useDepth;
    const bool previousWriteDepth = this->writeDepth;
    const bool previousUseBlending = this->useBlending;
    const opal::CullMode previousCullMode = this->cullMode;
    const opal::CompareOp previousDepthCompare = this->depthCompareOp;

    pipeline->enableBlending(false);
    pipeline->enableDepthTest(true);
    pipeline->enableDepthWrite(true);
    pipeline->setCullMode(opal::CullMode::Back);
    pipeline->setDepthCompareOp(opal::CompareOp::Less);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, false);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->writeDepth, true);
    updatePipelineStateField(this->cullMode, opal::CullMode::Back);
    updatePipelineStateField(this->depthCompareOp, opal::CompareOp::Less);

    commandBuffer->clear(this->clearColor.r, this->clearColor.g,
                         this->clearColor.b, this->clearColor.a, 1.0f);

    currentRenderTarget = &target;

    glm::mat4 view = this->camera->calculateViewMatrix();
    glm::mat4 projection = calculateProjectionMatrix();

    auto renderQueue = [&](const std::vector<Renderable *> &queue,
                           bool skipLate) {
        ShaderProgram oldProgram;
        for (auto *obj : queue) {
            if (obj == nullptr) {
                continue;
            }
            if (skipLate && obj->renderLateForward) {
                continue;
            }
            if (dynamic_cast<Fluid *>(obj) == &fluid) {
                continue;
            }
            if (obj->canUseDeferredRendering()) {
                oldProgram = obj->getShaderProgram().value();
                obj->setShader(ShaderProgram::fromDefaultShaders(
                    AtlasVertexShader::Main, AtlasFragmentShader::Main));
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));

            if (obj->canUseDeferredRendering()) {
                obj->setShader(oldProgram);
            }
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    if (previousTarget && previousTarget->getFramebuffer()) {
        previousTarget->bind();
        previousTarget->getFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    } else {
        device->getDefaultFramebuffer()->bind();
        device->getDefaultFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    }
    setViewportState(previousViewportX, previousViewportY,
                     previousViewportWidth, previousViewportHeight);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    pipeline->enableClipDistance(0, clipBackup);

    pipeline->enableBlending(previousUseBlending);
    pipeline->enableDepthTest(previousUseDepth);
    pipeline->enableDepthWrite(previousWriteDepth);
    pipeline->setCullMode(previousCullMode);
    pipeline->setDepthCompareOp(previousDepthCompare);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, previousUseBlending);
    updatePipelineStateField(this->useDepth, previousUseDepth);
    updatePipelineStateField(this->writeDepth, previousWriteDepth);
    updatePipelineStateField(this->cullMode, previousCullMode);
    updatePipelineStateField(this->depthCompareOp, previousDepthCompare);
}

void Window::markPipelineStateDirty() { ++pipelineStateVersion; }

bool Window::shouldRefreshPipeline(Renderable *renderable) {
    if (renderable == nullptr) {
        return false;
    }
    auto &version = renderablePipelineVersions[renderable];
    if (version != pipelineStateVersion) {
        version = pipelineStateVersion;
        return true;
    }
    return false;
}

void Window::setViewportState(int x, int y, int width, int height) {
    updatePipelineStateField(this->viewportX, x);
    updatePipelineStateField(this->viewportY, y);
    updatePipelineStateField(this->viewportWidth, width);
    updatePipelineStateField(this->viewportHeight, height);
}

void Window::updateBackbufferTarget(int width, int height) {
    if (!this->screenRenderTarget) {
        this->screenRenderTarget = std::make_unique<RenderTarget>();
        this->screenRenderTarget->type = RenderTargetType::Scene;
    }

    RenderTarget &target = *this->screenRenderTarget;
    target.texture.creationData.width = width;
    target.texture.creationData.height = height;
    target.depthTexture.creationData.width = width;
    target.depthTexture.creationData.height = height;
    target.type = RenderTargetType::Scene;
}
