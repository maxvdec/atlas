/*
 scene.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Scene functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_SCENE_H
#define ATLAS_SCENE_H

#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "bezel/body.h"
#include <vector>

class Window;

/**
 * @brief Abstract class that represents a 3D scene. It contains all lights and
 * objects that are going to be rendered. It also provides methods for updating
 * the scene and handling input events.
 * \subsection scene-example Example
 * ```cpp
 * class MyScene : public Scene {
 *  public:
 *    void initialize(Window &window) override {
 *      // Initialize scene objects and lights here
 *    }
 *
 *    void update(Window &window) override {
 *      // Update scene logic here
 *    }
 *   void onMouseMove(Window &window, Movement2d movement) override {
 *      // Handle mouse movement here
 *    }
 * };
 *
 * ```
 *
 */
class Scene {
  public:
    /**
     * @brief Function that initializes the scene. This method is called every
     * frame.
     *
     * @param window The window in which the scene is going to be runned.
     */
    virtual void update(Window &window) {};
    /**
     * @brief Function that initializes the scene. This method is called once by
     * the \ref Window class.
     *
     * @param window The window in which the scene is going to be runned.
     */
    virtual void initialize(Window &window) = 0;
    /**
     * @brief Function that handles mouse movement events.
     *
     * @param window  The window in which the scene is being rendered.
     * @param movement The movement delta.
     */
    virtual void onMouseMove(Window &window, Movement2d movement) {}
    /**
     * @brief Function that handles mouse scroll events.
     *
     * @param window The window in which the scene is being rendered.
     * @param offset The scroll offset.
     */
    virtual void onMouseScroll(Window &window, Movement2d offset) {}

    /**
     * @brief The ambient light in the scene.
     *
     */
    AmbientLight ambientLight = {{1.0f, 1.0f, 1.0f}, 0.1f};

    /**
     * @brief Function that adds a directional light to the scene. \warning The
     * light object must be valid during the entire scene lifetime.
     *
     * @param light The directional light to add.
     */
    void addDirectionalLight(DirectionalLight *light) {
        directionalLights.push_back(light);
    }

    /**
     * @brief Function that adds a point light to the scene. \warning The light
     * object must be valid during the entire scene lifetime.
     *
     * @param light The point light to add.
     */

    void addLight(Light *light) { pointLights.push_back(light); }

    /**
     * @brief Function that adds a spotlight to the scene. \warning The light
     * object must be valid during the entire scene lifetime.
     *
     * @param light The spotlight to add.
     */
    void addSpotlight(Spotlight *light) { spotlights.push_back(light); }

    void setSkybox(Skybox *newSkybox) { skybox = newSkybox; }

  private:
    std::vector<DirectionalLight *> directionalLights;
    std::vector<Light *> pointLights;
    std::vector<Spotlight *> spotlights;
    Skybox *skybox = nullptr;

    friend class CoreObject;
    friend class Window;
};

#endif // ATLAS_SCENE_H
