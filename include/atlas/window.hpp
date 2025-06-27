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

#include <atlas/units.hpp>
#include <string>

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
    Position2d position;

    Color backgroundColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    RenderingMode renderingMode = RenderingMode::Full;

  private:
    GLFWwindow *window;
};

#endif // ATLAS_WINDOW_HPP
