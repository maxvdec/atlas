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
#include <sys/types.h>
#include <vector>

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

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    TessellationControl,
    TessellationEvaluation
};

class Shader {
  public:
    static std::shared_ptr<Shader> createFromSource(const char *source,
                                                    ShaderType type);

    void compile();

    bool getShaderStatus() const;
    void getShaderLog(char *logBuffer, size_t bufferSize) const;

    uint shaderID;

  private:
#ifdef OPENGL
    static uint getGLShaderType(ShaderType type);
#endif
};

class ShaderProgram {
  public:
    static std::shared_ptr<ShaderProgram> create();
    void attachShader(std::shared_ptr<Shader> shader);

    void link();
    void use();

    bool getProgramStatus() const;
    void getProgramLog(char *logBuffer, size_t bufferSize) const;

    uint programID;
    std::vector<std::shared_ptr<Shader>> attachedShaders;
};

} // namespace opal

#endif // OPAL_H
