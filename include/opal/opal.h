/*
 opal.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main opal core rendering implementation
 Copyright (c) 2025 maxvdec
*/

#ifndef OPAL_H
#define OPAL_H

#include <memory>
#include <GLFW/glfw3.h>

namespace opal {

enum class OpenGLProfile { Core, Compatibility };

struct ContextConfiguration {
    bool useOpenGL = true;
    int majorVersion = 4;
    int minorVersion = 1;
    OpenGLProfile profile = OpenGLProfile::Core;
};

class Context {
  public:
    static std::shared_ptr<Context> create(ContextConfiguration config = {});

    void setFlag(int flag, bool enabled);
    void setFlag(int flag, int value);

    GLFWwindow *makeWindow(int width, int height, const char *title,
                           GLFWmonitor *monitor = nullptr,
                           GLFWwindow *share = nullptr);
    GLFWwindow *getWindow() const;

    void makeCurrent();

  private:
    GLFWwindow *window = nullptr;
};

class Device {
  public:
    static std::shared_ptr<Device> acquire(std::shared_ptr<Context> context);
};

} // namespace opal

#endif // OPAL_H
