/*
 window.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window creation and customization functions
 Copyright (c) 2025 maxvdec
*/

#ifndef WINDOW_H
#define WINDOW_H

#include "atlas/camera.h"
#include "atlas/input.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

typedef void *CoreWindowReference;
typedef void *CoreMonitorReference;

constexpr int WINDOW_CENTERED = -1;
constexpr int DEFAULT_ASPECT_RATIO = -1;

struct WindowConfiguration {
    std::string title;
    int width;
    int height;
    bool mouseCaptured = true;
    int posX = WINDOW_CENTERED;
    int posY = WINDOW_CENTERED;
    bool multisampling = true;
    bool decorations = true;
    bool resizable = true;
    bool transparent = false;
    bool alwaysOnTop = false;
    float opacity = 1.0f;
    int aspectRatioX = -1;
    int aspectRatioY = -1;
};

struct VideoMode {
    int width;
    int height;
    int refreshRate;
};

class Monitor {
  public:
    int monitorID;
    bool primary;

    std::vector<VideoMode> queryVideoModes();
    VideoMode getCurrentVideoMode();

    std::tuple<int, int> getPhysicalSize(); // in millimeters
    std::tuple<int, int> getPosition();
    std::tuple<float, float> getContentScale();
    std::string getName();

    Monitor(CoreMonitorReference ref, int id, bool isPrimary);

    CoreMonitorReference monitorRef;
};

class ShaderProgram;

class Window {
  public:
    std::string title;
    int width;
    int height;

    Window(WindowConfiguration config);
    ~Window();

    void run();
    void close();
    void setFullscreen(bool enable);
    void setFullscreen(Monitor &monitor);
    void setWindowed(WindowConfiguration config);
    std::vector<Monitor> static enumerateMonitors();

    void addObject(Renderable *object);
    void addPreferencedObject(Renderable *object);
    inline void addPreludeObject(Renderable *object) {
        firstRenderables.push_back(object);
    }

    void setCamera(Camera *newCamera);
    void setScene(Scene *scene);

    float getTime();

    bool isKeyPressed(Key key);

    void releaseMouse();
    void captureMouse();

    std::tuple<int, int> getCursorPosition();

    static Window *mainWindow;

    inline Scene *getCurrentScene() { return currentScene; }
    inline Camera *getCamera() { return camera; }
    void addRenderTarget(RenderTarget *target);

    inline Size2d getSize() {
        int fbw, fbh;
        glfwGetFramebufferSize(static_cast<GLFWwindow *>(windowRef), &fbw,
                               &fbh);
        return {static_cast<double>(fbw), static_cast<double>(fbh)};
    }

    inline void activateDebug() { this->debug = true; }
    inline void deactivateDebug() { this->debug = false; }

  private:
    CoreWindowReference windowRef;
    std::vector<Renderable *> renderables;
    std::vector<Renderable *> preferenceRenderables;
    std::vector<Renderable *> firstRenderables;
    std::vector<RenderTarget *> renderTargets;

    glm::mat4 calculateProjectionMatrix();
    Scene *currentScene = nullptr;

    void renderLightsToShadowMaps();
    Size2d getFurthestPositions();

    Camera *camera = nullptr;
    float lastMouseX;
    float lastMouseY;

    float lastTime = 0.0f;
    int frameCount = 0;

    ShaderProgram depthProgram;

    bool debug = false;

    friend class CoreObject;
};

#endif // WINDOW_H
