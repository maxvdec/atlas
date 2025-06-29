/*
 window.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The window logic for applications
 Copyright (c) 2025 maxvdec
*/

#include <glad/glad.h>

#include "atlas/core/rendering.hpp"
#include "atlas/input.hpp"
#include "atlas/units.hpp"
#include <GLFW/glfw3.h>
#include <atlas/window.hpp>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

Window *Window::current_window = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (Window::current_window->firstMouse) {
        Window::current_window->firstMouse = false;
        MousePacket firstData;
        firstData.xpos = static_cast<float>(xpos);
        firstData.ypos = static_cast<float>(ypos);
        firstData.constrainPitch = true;
        for (auto &interactive : Window::current_window->interactiveObjects) {
            interactive->lastMouseData = firstData;
        }
        return;
    }

    for (auto &interactive : Window::current_window->interactiveObjects) {
        MousePacket data;
        float xoffset =
            static_cast<float>(xpos) - interactive->lastMouseData.xpos;
        float yoffset = interactive->lastMouseData.ypos -
                        static_cast<float>(ypos); // Invert y-axis
        data.xpos = static_cast<float>(xpos);
        data.ypos = static_cast<float>(ypos);
        data.xoffset = xoffset;
        data.yoffset = yoffset;
        data.constrainPitch = true;
        interactive->onMouseMove(data, Window::current_window->deltaTime);
        interactive->lastMouseData = data;
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        Key pressedKey = static_cast<Key>(key);
        for (auto &interactive : Window::current_window->interactiveObjects) {
            interactive->onKeyPress(pressedKey,
                                    Window::current_window->deltaTime);
        }
    } else if (action == GLFW_RELEASE) {
        Key releasedKey = static_cast<Key>(key);
        for (auto &interactive : Window::current_window->interactiveObjects) {
            interactive->onKeyRelease(releasedKey,
                                      Window::current_window->deltaTime);
        }
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
    MouseButton mouseButton = static_cast<MouseButton>(button);
    if (action == GLFW_PRESS) {
        for (auto &interactive : Window::current_window->interactiveObjects) {
            interactive->onMouseButtonPress(mouseButton,
                                            Window::current_window->deltaTime);
        }
    }
}

void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    MouseScrollPacket scrollData;
    scrollData.xoffset = static_cast<float>(xoffset);
    scrollData.yoffset = static_cast<float>(yoffset);

    for (auto &interactive : Window::current_window->interactiveObjects) {
        interactive->onMouseScroll(scrollData,
                                   Window::current_window->deltaTime);
    }
}

Window::Window(const std::string &title, Frame mesures, Position2d position) {
    this->size = mesures;
    this->position = position;
    this->mainCam = nullptr;
    current_window = this;

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    this->window = glfwCreateWindow(mesures.width, mesures.height,
                                    title.c_str(), nullptr, nullptr);
    if (this->window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(this->window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(this->window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    this->framebufferSize = Frame(framebufferWidth, framebufferHeight);

    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);
    glfwSetCursorPosCallback(this->window, mouse_callback);
    glfwSetKeyCallback(this->window, key_callback);
    glfwSetMouseButtonCallback(this->window, mouse_button_callback);
    glfwSetScrollCallback(this->window, mouse_scroll_callback);

    const char *renderer =
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << "Renderer: " << (renderer ? renderer : "Unknown")
              << "\nVersion: " << (version ? version : "Unknown") << std::endl;
}

void Window::run() {
    double previousTime = glfwGetTime();
    if (this->currentScene != nullptr) {
        this->currentScene->init();
        this->registerInteractive(this->currentScene);
    }

    while (!glfwWindowShouldClose(this->window)) {
        double currentTime = glfwGetTime();
        this->frameCount++;

        if (currentTime - previousTime >= 1.0) {
            double fps = static_cast<double>(this->frameCount) /
                         (currentTime - previousTime);
            std::cout << "FPS: " << fps << std::endl;
            this->frameCount = 0;
            previousTime = currentTime;
        }

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        for (auto &interactive : this->interactiveObjects) {
            interactive->atEachFrame(deltaTime);
        }

        for (auto &object : Renderer::instance().registeredObjects) {
            if (this->mainCam != nullptr) {
                object->viewMatrix = this->mainCam->getViewMatrix();
                object->projectionMatrix = this->mainCam->getProjectionMatrix(
                    static_cast<float>(this->size.width) /
                    static_cast<float>(this->size.height));
            } else {
                object->viewMatrix = glm::mat4(1.0f);
            }
        }

        glEnable(GL_DEPTH_TEST);
        glClearColor(this->backgroundColor.r, this->backgroundColor.g,
                     this->backgroundColor.b, this->backgroundColor.a);

        switch (this->renderingMode) {
        case RenderingMode::Full:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case RenderingMode::Points:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case RenderingMode::Lines:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer::instance().dispatchAll();

        glfwSwapBuffers(this->window);
        glfwPollEvents();
    }

    glfwDestroyWindow(this->window);
    glfwTerminate();
    this->window = nullptr;
    return;
}
