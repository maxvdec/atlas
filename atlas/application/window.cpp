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
#include <iostream>
#include <memory>
#include <optional>
#include <string>
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DECORATED, config.decorations ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,
                   config.transparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, config.alwaysOnTop ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.multisampling ? 4 : 0);

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

    Window::mainWindow = this;

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int w, int h) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
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

    float fpsAccumulator = 0.0f;
    float fpsTimer = 0.0f;
    int frameCount = 0;
    this->framesPerSecond = 60.0f;

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

        fpsAccumulator += this->deltaTime;
        frameCount++;

        if (fpsAccumulator >= 1.0f) {
            this->framesPerSecond = frameCount;
            frameCount = 0;
            fpsAccumulator = 0.0f;
        }

        // Update the renderables
        for (auto &obj : this->renderables) {
            obj->update(*this);
        }

        currentScene->update(*this);

        auto start = std::chrono::high_resolution_clock::now();

        renderLightsToShadowMaps();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        // Render to the targets
        for (auto &target : this->renderTargets) {
            glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
            if (target->brightTexture.id != 0) {
                unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0,
                                               GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, attachments);
            }

            if (this->usesDeferred) {
                unsigned int attachments[4] = {
                    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                    GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
                glDrawBuffers(4, attachments);
                this->deferredRendering(target);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer->fbo);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->fbo);
                glBlitFramebuffer(0, 0, gBuffer->texture.creationData.width,
                                  gBuffer->texture.creationData.height, 0, 0,
                                  target->texture.creationData.width,
                                  target->texture.creationData.height,
                                  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);

                GLenum targetAttachments[2] = {GL_COLOR_ATTACHMENT0,
                                               GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, targetAttachments);

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LEQUAL);
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
            Size2d windowSize = getSize();
            glViewport(0, 0, windowSize.width, windowSize.height);
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
    int posX = config.posX != WINDOW_CENTERED ? config.posX : 100;
    int posY = config.posY != WINDOW_CENTERED ? config.posY : 100;
    glfwSetWindowMonitor(window, nullptr, posX, posY, width, height, 0);
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

    int blurWidth = target->brightTexture.creationData.width;
    int blurHeight = target->brightTexture.creationData.height;

    if (this->pingpongFBOs[0] == 0 || this->pingpongFBOs[1] == 0) {
        glGenFramebuffers(2, this->pingpongFBOs);
        glGenTextures(2, this->pingpongBuffers);

        for (unsigned int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->pingpongFBOs[i]);
            glBindTexture(GL_TEXTURE_2D, this->pingpongBuffers[i]);
            // Use bright texture dimensions (downscaled)
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

    bool horizontal = true, firstIteration = true;
    int amount = 10;
    ShaderProgram blurShader = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Fullscreen, AtlasFragmentShader::GaussianBlur);
    ShaderProgram targetProgram = target->object->getShaderProgram().value();
    target->object->setShader(blurShader);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, blurWidth, blurHeight);

    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, this->pingpongFBOs[horizontal]);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(blurShader.programId);

        blurShader.setUniform1i("horizontal", horizontal ? 1 : 0);
        blurShader.setUniform1f("radius", 2.5f);

        glActiveTexture(GL_TEXTURE0);
        GLuint textureToSample = firstIteration
                                     ? target->brightTexture.id
                                     : this->pingpongBuffers[!horizontal];
        glBindTexture(GL_TEXTURE_2D, textureToSample);

        blurShader.setUniform1i("image", 0);

        glBindVertexArray(target->object->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, target->object->ebo);

        if (!target->object->indices.empty()) {
            glDrawElements(GL_TRIANGLES, target->object->indices.size(),
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, target->object->vertices.size());
        }

        glBindVertexArray(0);
        horizontal = !horizontal;
        firstIteration = false;
    }

    target->object->setShader(targetProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);

    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width =
        target->brightTexture.creationData.width;
    target->blurredTexture.creationData.height =
        target->brightTexture.creationData.height;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id = this->pingpongBuffers[!horizontal];
}

void Window::useDeferredRendering() {
    this->usesDeferred = true;
    auto target = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::GBuffer));
    target->initialize();
    this->gBuffer = target;
}

void Window::deferredRendering(RenderTarget *target) {
    // Render to G-Buffer
    std::vector<ShaderProgram> originalPrograms;
    for (auto &obj : this->renderables) {
        if (!obj->canUseDeferredRendering()) {
            continue;
        }
        if (obj->getShaderProgram() != std::nullopt) {
            originalPrograms.push_back(obj->getShaderProgram().value());
        } else {
            originalPrograms.push_back(ShaderProgram());
        }
        obj->setShader(this->deferredProgram);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, this->gBuffer->fbo);
    unsigned int attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (auto &obj : this->renderables) {
        if (obj->canUseDeferredRendering()) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime());
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    unsigned int targetAttachments[2] = {GL_COLOR_ATTACHMENT0,
                                         GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, targetAttachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    static Id quadVAO = 0;
    static Id quadVBO;
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
    }

    glUseProgram(this->lightProgram.programId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gPosition.id);
    this->lightProgram.setUniform1i("gPosition", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gNormal.id);
    this->lightProgram.setUniform1i("gNormal", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gAlbedoSpec.id);
    this->lightProgram.setUniform1i("gAlbedoSpec", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gMaterial.id);
    this->lightProgram.setUniform1i("gMaterial", 3);

    int boundTextures = 4;
    int boundCubemaps = 0;

    ShaderProgram shaderProgram = this->lightProgram;
    Scene *scene = this->currentScene;

    shaderProgram.setUniform3f("cameraPosition", getCamera()->position.x,
                               getCamera()->position.y,
                               getCamera()->position.z);

    // Set ambient light
    shaderProgram.setUniform4f(
        "ambientLight.color", scene->ambientLight.color.r,
        scene->ambientLight.color.g, scene->ambientLight.color.b, 1.0f);
    shaderProgram.setUniform1f("ambientLight.intensity",
                               scene->ambientLight.intensity / 4);

    // Set camera position
    shaderProgram.setUniform3f("cameraPosition", getCamera()->position.x,
                               getCamera()->position.y,
                               getCamera()->position.z);

    // Send directional lights
    int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
    shaderProgram.setUniform1i("directionalLightCount", dirLightCount);

    for (int i = 0; i < dirLightCount; i++) {
        DirectionalLight *light = scene->directionalLights[i];
        std::string baseName = "directionalLights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".direction", light->direction.x,
                                   light->direction.y, light->direction.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);
    }

    // Send point lights

    int pointLightCount = std::min((int)scene->pointLights.size(), 256);
    shaderProgram.setUniform1i("pointLightCount", pointLightCount);

    for (int i = 0; i < pointLightCount; i++) {
        Light *light = scene->pointLights[i];
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".position", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);

        PointLightConstants plc = light->calculateConstants();
        shaderProgram.setUniform1f(baseName + ".constant", plc.constant);
        shaderProgram.setUniform1f(baseName + ".linear", plc.linear);
        shaderProgram.setUniform1f(baseName + ".quadratic", plc.quadratic);
    }

    // Send spotlights

    int spotlightCount = std::min((int)scene->spotlights.size(), 256);
    shaderProgram.setUniform1i("spotlightCount", spotlightCount);

    for (int i = 0; i < spotlightCount; i++) {
        Spotlight *light = scene->spotlights[i];
        std::string baseName = "spotlights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".position", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform3f(baseName + ".direction", light->direction.x,
                                   light->direction.y, light->direction.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);
        shaderProgram.setUniform1f(baseName + ".cutOff", light->cutOff);
        shaderProgram.setUniform1f(baseName + ".outerCutOff",
                                   light->outerCutoff);
    }

    for (int i = 0; i < 5; i++) {
        std::string uniformName = "cubeMap" + std::to_string(i + 1);
        shaderProgram.setUniform1i(uniformName, i + 10);
    }

    int boundParameters = 0;

    // Cycle though directional lights
    for (auto light : scene->directionalLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (boundTextures >= 16) {
            break;
        }

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i(baseName + ".textureIndex", boundTextures);
        ShadowParams shadowParams =
            light->calculateLightSpaceMatrix(Window::mainWindow->renderables);
        shaderProgram.setUniformMat4f(baseName + ".lightView",
                                      shadowParams.lightView);
        shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                      shadowParams.lightProjection);
        shaderProgram.setUniform1f(baseName + ".bias", shadowParams.bias);
        shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

        boundParameters++;
        boundTextures++;
    }

    // Cycle though spotlights
    for (auto light : scene->spotlights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (boundTextures >= 16) {
            break;
        }

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i(baseName + ".textureIndex", boundTextures);
        std::tuple<glm::mat4, glm::mat4> lightSpace =
            light->calculateLightSpaceMatrix();
        shaderProgram.setUniformMat4f(baseName + ".lightView",
                                      std::get<0>(lightSpace));
        shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                      std::get<1>(lightSpace));
        shaderProgram.setUniform1f(baseName + ".bias", 0.005f);
        shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

        boundParameters++;
        boundTextures++;
    }

    for (auto light : scene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (boundTextures + 6 >= 16) {
            break;
        }

        glActiveTexture(GL_TEXTURE0 + 10 + boundCubemaps);

        glBindTexture(GL_TEXTURE_CUBE_MAP,
                      light->shadowRenderTarget->texture.id);
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i(baseName + ".textureIndex", boundCubemaps);
        shaderProgram.setUniform1f(baseName + ".farPlane", light->distance);
        shaderProgram.setUniform3f(baseName + ".lightPos", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform1i(baseName + ".isPointLight", 1);

        boundParameters++;
        boundTextures += 6;
    }

    shaderProgram.setUniform1i("shadowParamCount", boundParameters);

    GLint units[16];
    for (int i = 0; i < boundTextures; i++)
        units[i] = i;

    glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                 boundTextures, units);

    // Bind skybox
    if (scene->skybox != nullptr) {
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene->skybox->cubemap.id);
        shaderProgram.setUniform1i("skybox", boundTextures);
        boundTextures++;
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}