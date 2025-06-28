/*
 window.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The window logic for applications
 Copyright (c) 2025 maxvdec
*/

#include <glad/gl.h>

#include "atlas/core/rendering.hpp"
#include "atlas/units.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <atlas/window.hpp>
#include <cstddef>
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

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, this->size.width, this->size.height);

    glfwSetFramebufferSizeCallback(this->window, framebuffer_size_callback);
}

void Window::run() {
    while (!glfwWindowShouldClose(this->window)) {
        glClearColor(this->backgroundColor.r, this->backgroundColor.g,
                     this->backgroundColor.b, this->backgroundColor.a);

        switch (this->renderingMode) {
        case RenderingMode::Full:
            break;
        case RenderingMode::Points:
            glPointSize(5.0f);
            break;
        case RenderingMode::Lines:
            glLineWidth(2.0f);
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
