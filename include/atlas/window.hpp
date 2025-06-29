/*
 window.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The Window struct and features for the engine
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_WINDOW_HPP
#define ATLAS_WINDOW_HPP

#include "atlas/camera.hpp"
#include "atlas/input.hpp"
#include <atlas/units.hpp>
#include <string>
#include <vector>

struct GLFWwindow;

enum class RenderingMode {
    Full,
    Points,
    Lines,
};

class Window {
  public:
    Window(const std::string &title, Frame mesures,
           Position2d position = Position2d(0, 0));

    void run();

    Frame size;
    Frame framebufferSize;
    Position2d position;
    Camera *mainCam;

    Color backgroundColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    Color ambientColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    RenderingMode renderingMode = RenderingMode::Full;
    std::vector<Interactive *> interactiveObjects;
    bool firstMouse = true;

    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    int frameCount = 0;

    inline void registerInteractive(Interactive *object) {
        this->interactiveObjects.push_back(object);
    }

    inline void setMainCamera(Camera *cam) { this->mainCam = cam; }

    static Window *current_window;
    GLFWwindow *window;
};

#endif // ATLAS_WINDOW_HPP
