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
    int posX = WINDOW_CENTERED;
    int posY = WINDOW_CENTERED;
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

  private:
    CoreWindowReference windowRef;
};

#endif // WINDOW_H
