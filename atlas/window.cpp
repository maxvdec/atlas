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
#include "atlas/units.hpp"
#include <GLFW/glfw3.h>
#include <atlas/window.hpp>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

Window::Window(const std::string &title, Frame mesures, Position2d position) {
    this->size = mesures;
    this->position = position;

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

    glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);

    const char *renderer =
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << "Renderer: " << (renderer ? renderer : "Unknown")
              << "\nVersion: " << (version ? version : "Unknown") << std::endl;
}

void Window::run() {
    while (!glfwWindowShouldClose(this->window)) {
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
