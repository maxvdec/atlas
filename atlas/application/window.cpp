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
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "hydra/fluid.h"
#include "bezel/body.h"
#include "finewave/audio.h"
#include <atlas/window.h>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <tuple>
#include <opal/opal.h>

Window *Window::mainWindow = nullptr;

Window::Window(WindowConfiguration config)
    : title(config.title), width(config.width), height(config.height) {
    auto context = opal::Context::create();

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

    auto device = opal::Device::acquire(context);

    glfwSetWindowOpacity(window, config.opacity);
    glfwSetInputMode(window, GLFW_CURSOR,
                     config.mouseCaptured ? GLFW_CURSOR_DISABLED
                                          : GLFW_CURSOR_NORMAL);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

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
        glViewport(0, 0, fbWidth, fbHeight);
        if (Window::mainWindow != nullptr) {
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

    VertexShader pointVertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::PointLightShadow);
    pointVertexShader.compile();
    FragmentShader pointFragmentShader = FragmentShader::fromDefaultShader(
        AtlasFragmentShader::PointLightShadow);
    pointFragmentShader.compile();
    GeometryShader geometryShader = GeometryShader::fromDefaultShader(
        AtlasGeometryShader::PointLightShadow);
    geometryShader.compile();
    ShaderProgram pointProgram = ShaderProgram();
    pointProgram.vertexShader = pointVertexShader;
    pointProgram.fragmentShader = pointFragmentShader;
    pointProgram.geometryShader = geometryShader;
    pointProgram.compile();
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
        throw std::runtime_error("Failed to initialize audio engine");
    }
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

    this->lastTime = static_cast<float>(glfwGetTime());

    glEnable(GL_MULTISAMPLE);
    this->useDepth = true;
    this->useBlending = true;
    this->srcBlend = opal::BlendFunc::SrcAlpha;
    this->dstBlend = opal::BlendFunc::OneMinusSrcAlpha;

    this->framesPerSecond = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        if (this->currentScene == nullptr) {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
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

        renderLightsToShadowMaps();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        this->cullMode = opal::CullMode::None;
        this->cullMode = opal::CullMode::Back;
        // Render to the targets
        for (auto &target : this->renderTargets) {
            this->currentRenderTarget = target;
            int targetWidth = target->getWidth();
            int targetHeight = target->getHeight();
            this->depthCompareOp = opal::CompareOp::Less;
            this->writeDepth = true;
            this->cullMode = opal::CullMode::Back;

            glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
            this->viewportX = 0;
            this->viewportY = 0;
            this->viewportWidth = targetWidth;
            this->viewportHeight = targetHeight;
            if (target->brightTexture.id != 0) {
                unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0,
                                               GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, attachments);
            }

            if (this->usesDeferred) {
                this->deferredRendering(target);

                glBindFramebuffer(GL_READ_FRAMEBUFFER, this->gBuffer->fbo);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->fbo);
                glBlitFramebuffer(
                    0, 0, this->gBuffer->gPosition.creationData.width,
                    this->gBuffer->gPosition.creationData.height, 0, 0,
                    targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, target->fbo);
                unsigned int targetAttachments[2] = {GL_COLOR_ATTACHMENT0,
                                                     GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, targetAttachments);

                this->useDepth = true;
                this->depthCompareOp = opal::CompareOp::Less;
                this->writeDepth = true;
                this->cullMode = opal::CullMode::Back;

                for (auto &obj : this->firstRenderables) {
                    obj->setViewMatrix(this->camera->calculateViewMatrix());
                    obj->setProjectionMatrix(calculateProjectionMatrix());
                    obj->render(getDeltaTime());
                }

                for (auto &obj : this->renderables) {
                    if (obj->renderLateForward) {
                        continue;
                    }
                    if (!obj->canUseDeferredRendering()) {
                        obj->setViewMatrix(this->camera->calculateViewMatrix());
                        obj->setProjectionMatrix(calculateProjectionMatrix());
                        obj->render(getDeltaTime());
                    }
                }

                for (auto &obj : this->lateForwardRenderables) {
                    obj->setViewMatrix(this->camera->calculateViewMatrix());
                    obj->setProjectionMatrix(calculateProjectionMatrix());
                    obj->render(getDeltaTime());
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                target->resolve();
                continue;
            }
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }

            for (auto &obj : this->renderables) {
                if (obj->renderLateForward) {
                    continue;
                }
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }
            updateFluidCaptures();
            for (auto &obj : this->lateForwardRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            target->resolve();
        }

        // Render to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        this->viewportX = 0;
        this->viewportY = 0;
        this->viewportWidth = fbWidth;
        this->viewportHeight = fbHeight;
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (this->renderTargets.size() == 0) {
            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }

            for (auto &obj : this->renderables) {
                if (obj->renderLateForward) {
                    continue;
                }
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }

            updateFluidCaptures();

            for (auto &obj : this->lateForwardRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }
        }

        this->cullMode = opal::CullMode::None;
        for (auto &obj : this->preferenceRenderables) {
            RenderTarget *target = dynamic_cast<RenderTarget *>(obj);
            if (target != nullptr && target->brightTexture.id != 0) {
                this->renderPhysicalBloom(target);
            }
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime());
        }

        this->cullMode = opal::CullMode::Back;
        this->useBlending = true;
        for (auto &obj : this->uiRenderables) {
            obj->render(getDeltaTime());
        }

        this->lastViewMatrix = this->camera->calculateViewMatrix();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Window::addObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }

    this->renderables.push_back(obj);

    if (obj->renderLateForward) {
        addLateForwardObject(obj);
    }
}

void Window::addLateForwardObject(Renderable *object) {
    if (object == nullptr) {
        return;
    }

    if (std::find(lateForwardRenderables.begin(), lateForwardRenderables.end(),
                  object) == lateForwardRenderables.end()) {
        lateForwardRenderables.push_back(object);

        if (auto *fluid = dynamic_cast<Fluid *>(object)) {
            if (std::find(lateFluids.begin(), lateFluids.end(), fluid) ==
                lateFluids.end()) {
                lateFluids.push_back(fluid);
            }
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
    if (!this->camera->useOrthographic) {
        int fbWidth, fbHeight;
        GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        return glm::perspective(glm::radians(camera->fov), aspectRatio,
                                camera->nearClip, camera->farClip);
    } else {
        float orthoSize = this->camera->orthographicSize;
        int fbWidth, fbHeight;
        GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        return glm::ortho(-orthoSize * aspectRatio, orthoSize * aspectRatio,
                          -orthoSize, orthoSize, camera->nearClip,
                          camera->farClip);
    }
}

void Window::setFullscreen(bool enable) {
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
    if (this->pingpongFBOs[0] != 0 || this->pingpongFBOs[1] != 0) {
        glDeleteFramebuffers(2, this->pingpongFBOs);
        glDeleteTextures(2, this->pingpongBuffers);
        this->pingpongFBOs[0] = this->pingpongFBOs[1] = 0;
        this->pingpongBuffers[0] = this->pingpongBuffers[1] = 0;
        this->pingpongWidth = 0;
        this->pingpongHeight = 0;
    }
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

void Window::renderLightsToShadowMaps() {
    if (this->currentScene == nullptr) {
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

        depthPipeline = this->depthProgram.requestPipeline(depthPipeline);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);
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
            obj->render(getDeltaTime());
        }

        for (auto &obj : this->lateForwardRenderables) {
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);
            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime());
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
        spotlightsPipeline =
            this->depthProgram.requestPipeline(spotlightsPipeline);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);
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
            obj->render(getDeltaTime());
        }

        for (auto &obj : this->lateForwardRenderables) {
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);
            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime());
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
        pointLightPipeline =
            this->pointDepthProgram.requestPipeline(pointLightPipeline);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }
            std::vector<glm::mat4> shadowTransforms =
                light->calculateShadowTransforms();

            this->pointDepthProgram.setUniform3f("lightPos", light->position.x,
                                                 light->position.y,
                                                 light->position.z);
            this->pointDepthProgram.setUniform1f("far_plane", light->distance);
            light->lastShadowParams.farPlane = light->distance;
            for (size_t i = 0; i < shadowTransforms.size(); ++i) {
                this->pointDepthProgram.setUniformMat4f(
                    "shadowMatrices[" + std::to_string(i) + "]",
                    shadowTransforms[i]);
            }

            obj->setProjectionMatrix(glm::mat4(1.0));
            obj->setViewMatrix(glm::mat4(1.0));
            obj->setPipeline(pointLightPipeline);

            obj->render(getDeltaTime());
        }

        for (auto &obj : this->lateForwardRenderables) {
            if (obj->getPipeline() == std::nullopt || !obj->canCastShadows()) {
                continue;
            }
            std::vector<glm::mat4> shadowTransforms =
                light->calculateShadowTransforms();

            this->pointDepthProgram.setUniform3f("lightPos", light->position.x,
                                                 light->position.y,
                                                 light->position.z);
            this->pointDepthProgram.setUniform1f("far_plane", light->distance);
            light->lastShadowParams.farPlane = light->distance;
            for (size_t i = 0; i < shadowTransforms.size(); ++i) {
                this->pointDepthProgram.setUniformMat4f(
                    "shadowMatrices[" + std::to_string(i) + "]",
                    shadowTransforms[i]);
            }

            obj->setProjectionMatrix(glm::mat4(1.0));
            obj->setViewMatrix(glm::mat4(1.0));
            obj->setPipeline(pointLightPipeline);

            obj->render(getDeltaTime());
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

    glDisable(GL_POLYGON_OFFSET_FILL);
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

    if (this->pingpongFBOs[0] == 0 || this->pingpongFBOs[1] == 0 ||
        blurWidth != this->pingpongWidth ||
        blurHeight != this->pingpongHeight) {
        if (this->pingpongFBOs[0] != 0 || this->pingpongFBOs[1] != 0) {
            glDeleteFramebuffers(2, this->pingpongFBOs);
            glDeleteTextures(2, this->pingpongBuffers);
        }

        glGenFramebuffers(2, this->pingpongFBOs);
        glGenTextures(2, this->pingpongBuffers);
        this->pingpongWidth = blurWidth;
        this->pingpongHeight = blurHeight;

        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->pingpongFBOs[i]);
            glBindTexture(GL_TEXTURE_2D, this->pingpongBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, blurWidth, blurHeight, 0,
                         GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, this->pingpongBuffers[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
                GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Pingpong Framebuffer not complete!" << std::endl;
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    bool horizontal = true;
    bool firstIteration = true;
    const unsigned int blurIterations = std::max(1u, this->bloomBlurPasses);

    ShaderProgram &blurShader = this->bloomBlurProgram;
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

    glUseProgram(blurShader.programId);
    blurShader.setUniform1f("radius", 2.5f);
    blurShader.setUniform1i("image", 0);

    glBindVertexArray(target->object->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, target->object->ebo);

    for (unsigned int i = 0; i < blurIterations; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, this->pingpongFBOs[horizontal]);
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader.setUniform1i("horizontal", horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        GLuint textureToSample = firstIteration
                                     ? target->brightTexture.id
                                     : this->pingpongBuffers[!horizontal];
        glBindTexture(GL_TEXTURE_2D, textureToSample);

        if (!target->object->indices.empty()) {
            glDrawElements(GL_TRIANGLES, target->object->indices.size(),
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, target->object->vertices.size());
        }

        horizontal = !horizontal;
        firstIteration = false;
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    target->object->setPipeline(targetProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);

    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width = this->pingpongWidth;
    target->blurredTexture.creationData.height = this->pingpongHeight;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id = this->pingpongBuffers[!horizontal];
}

void Window::useDeferredRendering() {
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

void Window::updateFluidCaptures() {
    for (auto *fluid : lateFluids) {
        if (fluid == nullptr) {
            continue;
        }
        if (fluid->captureDirty) {
            fluid->updateCapture(*this);
        }
    }
}

void Window::captureFluidReflection(Fluid &fluid) {
    if (!fluid.reflectionTarget) {
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
    glEnable(GL_CLIP_DISTANCE0);

    RenderTarget *previousTarget = currentRenderTarget;

    GLint previousFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
    auto pipeline = opal::Pipeline::create();

    glViewport(0, 0, target.getWidth(), target.getHeight());
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFaceMode = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
    GLboolean previousDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    GLfloat previousClearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glClearColor(fluid.color.r, fluid.color.g, fluid.color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                std::cout << "For object with type: " << typeid(*obj).name()
                          << " and ID: " << obj->getId()
                          << " setting reflection shader." << std::endl;
                obj->setShader(ShaderProgram::fromDefaultShaders(
                    AtlasVertexShader::Main, AtlasFragmentShader::Main));
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime());

            if (obj->canUseDeferredRendering()) {
                obj->setShader(oldProgram);
            }
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    glBindFramebuffer(GL_FRAMEBUFFER, previousFbo);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2],
               previousViewport[3]);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    if (clipBackup) {
        glEnable(GL_CLIP_DISTANCE0);
    } else {
        glDisable(GL_CLIP_DISTANCE0);
    }

    if (!cullEnabled) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(previousCullFaceMode);
    }

    glDepthMask(previousDepthMask);
    if (!depthTestEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
    glClearColor(previousClearColor[0], previousClearColor[1],
                 previousClearColor[2], previousClearColor[3]);

    this->camera = cameraBackup;
}

void Window::captureFluidRefraction(Fluid &fluid) {
    if (!fluid.refractionTarget) {
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
    glEnable(GL_CLIP_DISTANCE0);

    RenderTarget *previousTarget = currentRenderTarget;
    GLint previousFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
    glViewport(0, 0, target.getWidth(), target.getHeight());
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFaceMode = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
    GLboolean previousDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    GLfloat previousClearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(1.0, 0.0, 1.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                std::cout << "For object with type: " << typeid(*obj).name()
                          << " and ID: " << obj->getId()
                          << " setting refraction shader." << std::endl;
                obj->setShader(ShaderProgram::fromDefaultShaders(
                    AtlasVertexShader::Main, AtlasFragmentShader::Main));
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime());

            if (obj->canUseDeferredRendering()) {
                obj->setShader(oldProgram);
            }
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    glBindFramebuffer(GL_FRAMEBUFFER, previousFbo);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2],
               previousViewport[3]);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    if (clipBackup) {
        glEnable(GL_CLIP_DISTANCE0);
    } else {
        glDisable(GL_CLIP_DISTANCE0);
    }

    if (!cullEnabled) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(previousCullFaceMode);
    }

    glDepthMask(previousDepthMask);
    if (!depthTestEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
    glClearColor(previousClearColor[0], previousClearColor[1],
                 previousClearColor[2], previousClearColor[3]);
}
