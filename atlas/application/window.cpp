/*
 window.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <atlas/window.h>
#include <iostream>
#include <optional>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

    this->shadowRenderTarget =
        std::make_shared<RenderTarget>(*this, RenderTargetType::Shadow);
    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Depth);
    vertexShader.compile();
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Empty);
    fragmentShader.compile();
    ShaderProgram program;
    program.vertexShader = vertexShader;
    program.fragmentShader = fragmentShader;
    this->depthProgram = program;
}

std::tuple<int, int> Window::getCursorPosition() {
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return {static_cast<int>(xpos), static_cast<int>(ypos)};
}

void Window::run() {
    if (this->currentScene == nullptr) {
        throw std::runtime_error("No scene set for the window");
    }
    if (this->camera == nullptr) {
        this->camera = new Camera();
    }
    for (auto &obj : this->renderables) {
        obj->initialize();
    }
    for (auto &obj : this->preferenceRenderables) {
        obj->initialize();
    }
    GLFWwindow *window = static_cast<GLFWwindow *>(this->windowRef);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    while (!glfwWindowShouldClose(window)) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        frameCount++;

        // Update FPS
        if (getTime() - lastTime >= 1.0f) {
            std::string newTitle =
                title + " - FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, newTitle.c_str());
            frameCount = 0;
            lastTime = getTime();
        }

        currentScene->update(*this);

        glViewport(0, 0, shadowRenderTarget->texture.creationData.width,
                   shadowRenderTarget->texture.creationData.height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowRenderTarget->fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (auto &obj : this->renderables) {
            if (obj->getShaderProgram() == std::nullopt) {
                continue;
            }
            ShaderProgram program = obj->getShaderProgram().value();

            obj->setShader(this->depthProgram);
            float near_plane = 1.0f, far_plane = 7.5f;
            glm::mat4 lightProjection =
                glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
            glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f),
                                              glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            obj->setProjectionMatrix(lightSpaceMatrix);
            obj->render();
            obj->setShader(program);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render to the targets
        for (auto &target : this->renderTargets) {
            glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
            glViewport(0, 0, target->texture.creationData.width,
                       target->texture.creationData.height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto &obj : this->firstRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render();
            }

            for (auto &obj : this->renderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render();
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Render to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (this->renderTargets.size() == 0) {
            for (auto &obj : this->renderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render();
            }
        }

        glDisable(GL_CULL_FACE);
        for (auto &obj : this->preferenceRenderables) {
            obj->render();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Window::debugDisplayDepthMap() {
    if (this->shadowRenderTarget == nullptr) {
        throw std::runtime_error("No shadow render target available");
    }
    if (!this->debug) {
        return;
    }
    this->shadowRenderTarget->display(*this);
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
