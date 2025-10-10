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
#include "bezel/body.h"
#include "finewave/audio.h"
#include <memory>
#include <string>
#include <tuple>
#include <vector>

typedef void *CoreWindowReference;
typedef void *CoreMonitorReference;

constexpr int WINDOW_CENTERED = -1;
constexpr int DEFAULT_ASPECT_RATIO = -1;

/**
 * @brief Structure representing the configuration options for creating a
 * window.
 *
 */
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

/**
 * @brief Structure representing a video mode with resolution and refresh rate.
 *
 */
struct VideoMode {
    int width;
    int height;
    int refreshRate;
};

/**
 * @brief Class representing a monitor with video mode querying capabilities.
 *
 */
class Monitor {
  public:
    int monitorID;
    bool primary;

    /**
     * @brief Queries all available video modes for this monitor.
     *
     * @return (std::vector<VideoMode>) Vector of available video modes.
     */
    std::vector<VideoMode> queryVideoModes();
    /**
     * @brief Gets the current video mode of this monitor.
     *
     * @return (VideoMode) The current video mode.
     */
    VideoMode getCurrentVideoMode();

    /**
     * @brief Gets the physical size of the monitor in millimeters.
     *
     * @return (std::tuple<int, int>) Width and height in millimeters.
     */
    std::tuple<int, int> getPhysicalSize(); // in millimeters
    /**
     * @brief Gets the position of the monitor in the desktop coordinate system.
     *
     * @return (std::tuple<int, int>) X and Y position coordinates.
     */
    std::tuple<int, int> getPosition();
    /**
     * @brief Gets the content scale factors for this monitor.
     *
     * @return (std::tuple<float, float>) Horizontal and vertical scale factors.
     */
    std::tuple<float, float> getContentScale();
    /**
     * @brief Gets the human-readable name of this monitor.
     *
     * @return (std::string) The monitor name.
     */
    std::string getName();

    /**
     * @brief Constructs a Monitor object.
     *
     * @param ref Core monitor reference.
     * @param id Monitor ID.
     * @param isPrimary Whether this is the primary monitor.
     */
    Monitor(CoreMonitorReference ref, int id, bool isPrimary);

    CoreMonitorReference monitorRef;
};

class ShaderProgram;

/**
 * @brief Structure representing a window in the application. This contains the
 * main interface for interacting with the engine.
 * \subsection window-example Example
 * ```cpp
 * // Create a window with specific configuration
 * WindowConfiguration config;
 * config.title = "My Game";
 * config.width = 1280;
 * config.height = 720;
 * Window window(config);
 * // Set up camera and scene
 * Camera camera;
 * Scene scene;
 * window.setCamera(&camera);
 * window.setScene(&scene);
 * // Add objects to the scene
 * CoreObject obj;
 * scene.addObject(&obj);
 * // Run the main window loop
 * window.run();
 * ```
 *
 */
class Window {
  public:
    std::string title;
    int width;
    int height;

    /**
     * @brief Constructs a window with the specified configuration.
     *
     * @param config Window configuration settings.
     */
    Window(WindowConfiguration config);
    /**
     * @brief Destructor for Window.
     *
     */
    ~Window();

    /**
     * @brief Starts the main window loop and begins rendering.
     *
     */
    void run();
    /**
     * @brief Closes the window and terminates the application.
     *
     */
    void close();
    /**
     * @brief Sets the window to fullscreen mode.
     *
     * @param enable True to enable fullscreen, false to disable.
     */
    void setFullscreen(bool enable);
    /**
     * @brief Sets the window to fullscreen on a specific monitor.
     *
     * @param monitor The monitor to use for fullscreen.
     */
    void setFullscreen(Monitor &monitor);
    /**
     * @brief Sets the window to windowed mode with new configuration.
     *
     * @param config New window configuration.
     */
    void setWindowed(WindowConfiguration config);
    /**
     * @brief Enumerates all available monitors.
     *
     * @return (std::vector<Monitor>) Vector of available monitors.
     */
    std::vector<Monitor> static enumerateMonitors();

    /**
     * @brief Adds a renderable object to the window.
     *
     * @param object The renderable object to add. \warning The object must be
     * long-lived. This means that declaring it as a class property is a good
     * idea.
     */
    void addObject(Renderable *object);
    /**
     * @brief Adds a renderable object with higher rendering priority.
     *
     * @param object The renderable object to add with preference. \warning The
     * object must be long-lived. This means that declaring it as a class
     * property is a good idea.
     */
    void addPreferencedObject(Renderable *object);
    /**
     * @brief Adds a renderable object to be rendered first.
     *
     * @param object The renderable object to add to the prelude. \warning The
     * object must be long-lived. This means that declaring it as a class
     * property is a good idea.
     */
    inline void addPreludeObject(Renderable *object) {
        firstRenderables.push_back(object);
    }

    inline void addUIObject(Renderable *object) {
        uiRenderables.push_back(object);
    }

    /**
     * @brief Sets the camera for the window.
     *
     * @param newCamera The camera to use for rendering. \warning The camera
     * must be long-lived. This means that declaring it as a class property is
     * a good idea.
     */
    void setCamera(Camera *newCamera);
    /**
     * @brief Sets the scene for the window.
     *
     * @param scene The scene to render.
     */
    void setScene(Scene *scene);

    /**
     * @brief Gets the current time since window creation.
     *
     * @return (float) Time in seconds.
     */
    float getTime();

    /**
     * @brief Checks if a key is currently pressed.
     *
     * @param key The key to check.
     * @return (bool) True if the key is pressed, false otherwise.
     */
    bool isKeyPressed(Key key);
    /**
     * @brief Checks if a key was clicked (pressed and released) this frame.
     *
     * @param key The key to check.
     * @return (bool) True if the key was clicked, false otherwise.
     */
    bool isKeyClicked(Key key);

    /**
     * @brief Releases mouse capture, allowing the cursor to move freely.
     *
     */
    void releaseMouse();
    /**
     * @brief Captures the mouse cursor for camera control.
     *
     */
    void captureMouse();

    /**
     * @brief Gets the current cursor position.
     *
     * @return (std::tuple<int, int>) X and Y cursor coordinates.
     */
    std::tuple<int, int> getCursorPosition();

    static Window *mainWindow;

    /**
     * @brief Gets the current scene being rendered.
     *
     * @return (Scene*) Pointer to the current scene.
     */
    inline Scene *getCurrentScene() { return currentScene; }
    /**
     * @brief Gets the current camera.
     *
     * @return (Camera*) Pointer to the current camera.
     */
    inline Camera *getCamera() { return camera; }
    /**
     * @brief Adds a render target to the window.
     *
     * @param target The render target to add.
     */
    void addRenderTarget(RenderTarget *target);

    /**
     * @brief Gets the framebuffer size of the window.
     *
     * @return (Size2d) The width and height of the framebuffer.
     */
    inline Size2d getSize() {
        int fbw, fbh;
        glfwGetFramebufferSize(static_cast<GLFWwindow *>(windowRef), &fbw,
                               &fbh);
        return {static_cast<double>(fbw), static_cast<double>(fbh)};
    }

    /**
     * @brief Activates debug mode for the window.
     *
     */
    inline void activateDebug() { this->debug = true; }
    /**
     * @brief Deactivates debug mode for the window.
     *
     */
    inline void deactivateDebug() { this->debug = false; }

    /**
     * @brief Gets the delta time between frames.
     *
     * @return (float) Delta time in seconds.
     */
    inline float getDeltaTime() { return this->deltaTime; }
    /**
     * @brief Gets the current frames per second.
     *
     * @return (float) Frames per second value.
     */
    inline float getFramesPerSecond() { return this->framesPerSecond; }

    /**
     * @brief Gets all physics bodies in the window.
     *
     * @return (std::vector<std::shared_ptr<Body>>) Vector of all physics
     * bodies.
     */
    std::vector<std::shared_ptr<Body>> getAllBodies();

    float gravity = 9.81f;

    std::shared_ptr<AudioEngine> audioEngine;

  private:
    CoreWindowReference windowRef;
    std::vector<Renderable *> renderables;
    std::vector<Renderable *> preferenceRenderables;
    std::vector<Renderable *> firstRenderables;
    std::vector<Renderable *> uiRenderables;
    std::vector<RenderTarget *> renderTargets;

    glm::mat4 calculateProjectionMatrix();
    Scene *currentScene = nullptr;

    void renderLightsToShadowMaps();
    Size2d getFurthestPositions();

    void renderPingpong(RenderTarget *target, float dt);

    Camera *camera = nullptr;
    float lastMouseX;
    float lastMouseY;

    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    float framesPerSecond = 0.0f;

    ShaderProgram depthProgram;
    ShaderProgram pointDepthProgram;

    bool debug = false;

    unsigned int pingpongFBOs[2] = {0, 0};
    unsigned int pingpongBuffers[2] = {0, 0};

    friend class CoreObject;
    friend class RenderTarget;
    friend class DirectionalLight;
    friend class Text;
};

#endif // WINDOW_H
