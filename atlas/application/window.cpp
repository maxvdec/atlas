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
#include "bezel/body.h"
#include "finewave/audio.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <atlas/window.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <tuple>

Window *Window::mainWindow = nullptr;

Window::Window(WindowConfiguration config)
    : title(config.title), width(config.width), height(config.height) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DECORATED, config.decorations ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,
                   config.transparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, config.alwaysOnTop ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.multisampling ? 4 : 0);

#ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    GLFWwindow *window =
        glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

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

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int w, int h) {
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
        window, [](GLFWwindow *win, double xpos, double ypos) {
            Window *window = Window::mainWindow;
            Position2d movement = {xpos - window->lastMouseX,
                                   window->lastMouseY - ypos};
            if (window->currentScene != nullptr) {
                window->currentScene->onMouseMove(*window, movement);
            }
            window->lastMouseX = xpos;
            window->lastMouseY = ypos;
        });

    glfwSetScrollCallback(
        window, [](GLFWwindow *win, double xoffset, double yoffset) {
            Window *window = Window::mainWindow;
            Position2d offset = {xoffset, yoffset};
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
    for (auto &obj : this->uiRenderables) {
        obj->initialize();
    }
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);

    this->lastTime = static_cast<float>(glfwGetTime());

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

        // Update the renderables
        for (auto &obj : this->renderables) {
            obj->update(*this);
        }

        currentScene->update(*this);

        renderLightsToShadowMaps();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        // Render to the targets
        for (auto &target : this->renderTargets) {
            int targetWidth = target->getWidth();
            int targetHeight = target->getHeight();
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
            glViewport(0, 0, targetWidth, targetHeight);
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

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);

                for (auto &obj : this->firstRenderables) {
                    obj->setViewMatrix(this->camera->calculateViewMatrix());
                    obj->setProjectionMatrix(calculateProjectionMatrix());
                    obj->render(getDeltaTime());
                }

                for (auto &obj : this->renderables) {
                    if (!obj->canUseDeferredRendering()) {
                        obj->setViewMatrix(this->camera->calculateViewMatrix());
                        obj->setProjectionMatrix(calculateProjectionMatrix());
                        obj->render(getDeltaTime());
                    }
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
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (this->renderTargets.size() == 0) {
            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }

            for (auto &obj : this->renderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime());
            }
        }

        glDisable(GL_CULL_FACE);
        for (auto &obj : this->preferenceRenderables) {
            RenderTarget *target = dynamic_cast<RenderTarget *>(obj);
            if (target != nullptr && target->brightTexture.id != 0) {
                this->renderPingpong(target, getDeltaTime());
            }
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime());
        }

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        for (auto &obj : this->uiRenderables) {
            obj->render(getDeltaTime());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Window::addObject(Renderable *obj) { this->renderables.push_back(obj); }

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
    : monitorRef(ref), monitorID(id), primary(isPrimary) {}

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

    std::vector<ShaderProgram> originalPrograms;
    for (auto &obj : this->renderables) {
        if (obj->getShaderProgram() != std::nullopt) {
            originalPrograms.push_back(obj->getShaderProgram().value());
        } else {
            originalPrograms.push_back(ShaderProgram()); // Placeholder
        }
    }

    for (auto &light : this->currentScene->directionalLights) {
        if (light->doesCastShadows == false) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        glViewport(0, 0, shadowRenderTarget->texture.creationData.width,
                   shadowRenderTarget->texture.creationData.height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        ShadowParams lightParams =
            light->calculateLightSpaceMatrix(this->renderables);
        glm::mat4 lightView = lightParams.lightView;
        glm::mat4 lightProjection = lightParams.lightProjection;
        for (auto &obj : this->renderables) {
            if (obj->getShaderProgram() == std::nullopt ||
                !obj->canCastShadows()) {
                continue;
            }
            originalPrograms.push_back(obj->getShaderProgram().value());

            obj->setShader(this->depthProgram);

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime());
        }
    }

    for (auto &light : this->currentScene->spotlights) {
        if (light->doesCastShadows == false) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        glViewport(0, 0, shadowRenderTarget->texture.creationData.width,
                   shadowRenderTarget->texture.creationData.height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        std::tuple<glm::mat4, glm::mat4> lightSpace =
            light->calculateLightSpaceMatrix();
        glm::mat4 lightView = std::get<0>(lightSpace);
        glm::mat4 lightProjection = std::get<1>(lightSpace);
        for (auto &obj : this->renderables) {
            if (obj->getShaderProgram() == std::nullopt ||
                !obj->canCastShadows()) {
                continue;
            }
            ShaderProgram program = obj->getShaderProgram().value();

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime());
        }
    }

    for (auto &light : this->currentScene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        renderedShadows = true;
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        glViewport(0, 0, shadowRenderTarget->texture.creationData.width,
                   shadowRenderTarget->texture.creationData.height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        for (auto &obj : this->renderables) {
            if (obj->getShaderProgram() == std::nullopt ||
                !obj->canCastShadows()) {
                continue;
            }
            std::vector<glm::mat4> shadowTransforms =
                light->calculateShadowTransforms();

            this->pointDepthProgram.setUniform3f("lightPos", light->position.x,
                                                 light->position.y,
                                                 light->position.z);
            this->pointDepthProgram.setUniform1f("far_plane", light->distance);
            for (size_t i = 0; i < shadowTransforms.size(); ++i) {
                this->pointDepthProgram.setUniformMat4f(
                    "shadowMatrices[" + std::to_string(i) + "]",
                    shadowTransforms[i]);
            }

            obj->setProjectionMatrix(glm::mat4(1.0));
            obj->setViewMatrix(glm::mat4(1.0));
            obj->setShader(this->pointDepthProgram);

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

    if (!renderedShadows) {
        return;
    }

    size_t i = 0;
    for (auto &renderable : this->renderables) {
        if (renderable->getShaderProgram() != std::nullopt &&
            i < originalPrograms.size()) {
            renderable->setShader(originalPrograms[i++]);
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

void Window::renderPingpong(RenderTarget *target, float dt) {
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
    auto originalProgram = target->object->getShaderProgram();
    if (!originalProgram.has_value()) {
        return;
    }

    ShaderProgram targetProgram = originalProgram.value();
    target->object->setShader(blurShader);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, blurWidth, blurHeight);

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

    target->object->setShader(targetProgram);
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
