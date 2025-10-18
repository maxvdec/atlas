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
#include <algorithm>
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
     * @brief Sets the intensity of the ambient light in the scene.
     *
     * @param intensity The desired ambient light intensity. This value is
     * divided by 4 internally.
     */
    void setAmbientIntensity(float intensity) {
        this->ambientLight.intensity = intensity / 4;
        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
        }
    }

    /**
     * @brief Enables or disables automatic ambient coloring derived from the
     * scene's skybox.
     */
    void setAutomaticAmbient(bool enabled) {
        automaticAmbient = enabled;
        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
        }
    }

    /**
     * @brief Returns whether automatic ambient sampling is enabled.
     */
    bool isAutomaticAmbientEnabled() const { return automaticAmbient; }

    /**
     * @brief Gets the ambient color computed from the skybox when automatic
     * ambient is active.
     */
    Color getAutomaticAmbientColor() const { return automaticAmbientColor; }

    /**
     * @brief Gets the intensity derived from the skybox when automatic ambient
     * is active.
     */
    float getAutomaticAmbientIntensity() const {
        return automaticAmbientIntensity;
    }

    /**
     * @brief Returns the manually configured ambient light color.
     */
    Color getAmbientColor() const { return ambientLight.color; }

    /**
     * @brief Returns the manually configured ambient intensity.
     */
    float getAmbientIntensity() const { return ambientLight.intensity; }

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

    /**
     * @brief Set the Skybox object
     *
     * @param newSkybox The new skybox to set.
     */
    void setSkybox(Skybox *newSkybox) {
        skybox = newSkybox;
        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
        }
    }

  private:
    std::vector<DirectionalLight *> directionalLights;
    std::vector<Light *> pointLights;
    std::vector<Spotlight *> spotlights;
    Skybox *skybox = nullptr;
    AmbientLight ambientLight = {{1.0f, 1.0f, 1.0f, 1.0f}, 0.5f / 4};
    bool automaticAmbient = false;
    Color automaticAmbientColor = Color::white();
    float automaticAmbientIntensity = ambientLight.intensity;

    void updateAutomaticAmbientFromSkybox() {
        if (skybox != nullptr && skybox->cubemap.hasAverageColor) {
            automaticAmbientColor = skybox->cubemap.averageColor;
            double luminance = 0.2126 * automaticAmbientColor.r +
                               0.7152 * automaticAmbientColor.g +
                               0.0722 * automaticAmbientColor.b;
            automaticAmbientIntensity =
                static_cast<float>(std::clamp(luminance, 0.0, 1.0));
        } else {
            automaticAmbientColor = ambientLight.color;
            automaticAmbientIntensity = ambientLight.intensity;
        }
    }

    friend class CoreObject;
    friend class Window;
};

#endif // ATLAS_SCENE_H
