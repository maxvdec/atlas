/*
 light.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Light definition and concepts
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_LIGHT_H
#define ATLAS_LIGHT_H

#include "atlas/camera.h"
#include "atlas/core/renderable.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <memory>
#include <tuple>
#include <vector>

class Window;

/**
 * @brief Structure representing ambient light in a scene. This is the most
 * straightforward type of light.
 *
 */
struct AmbientLight {
    /**
     * @brief The color of the ambient light. This will be reflected into all
     * objects that allow so, to offer a cohesive ambient.
     *
     */
    Color color;
    /**
     * @brief The intensity with which the ambient light is applied.
     *
     */
    float intensity;
};

/**
 * @brief Mathematical constants for a point light.
 *
 */
struct PointLightConstants {
    /**
     * @brief The distance the light reaches.
     *
     */
    float distance;
    /**
     * @brief The constant attenuation factor, which determines how much the
     * light intensity decreases over distance.
     *
     */
    float constant;
    /**
     * @brief The linear attenuation factor.
     *
     */
    float linear;
    /**
     * @brief The quadratic attenuation factor.
     *
     */
    float quadratic;
    float radius;
};

/**
 * @brief Parameters that are submitted to a shadow shader to calculate shadows.
 *
 */
struct ShadowParams {
    /**
     * @brief The view matrix from the light's perspective.
     *
     */
    glm::mat4 lightView;
    /**
     * @brief The projection matrix from the light's perspective.
     *
     */
    glm::mat4 lightProjection;
    /**
     * @brief Constant bias to prevent shadow acne. It decreases when the
     * resolution increases.
     *
     */
    float bias;
};

/**
 * @brief Structure representing a point light in a scene. A point light emits
 * light in all directions from a single point in space.
 *
 * \subsection example-light Example
 * ```cpp
 * // Create a point light at position (10, 10, 10) with white color and a
 * distance of 50 units Light pointLight({10.0f, 10.0f, 10.0f},
 * Color::white(), 50.0f);
 * // Set the light color to a soft yellow
 * pointLight.setColor(Color(1.0f, 0.9f, 0.7f));
 * // Enable shadow casting for the light
 * pointLight.castShadows(window);
 * // Add a debug object to visualize the light in the scene
 * pointLight.createDebugObject();
 * pointLight.addDebugObject(window);
 * // Add the light to the scene
 * this->addPointLight(&pointLight);
 * ```
 */
struct Light {
    /**
     * @brief The position of the light in 3D space.
     *
     */
    Position3d position = {0.0f, 0.0f, 0.0f};

    /**
     * @brief The color of the light.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the light will use for specular highlights.
     *
     */
    Color shineColor = Color::white();

    /**
     * @brief Function that constructs a new Light object.
     *
     * @param pos The position of the light in 3D space.
     * @param color The color for the light.
     * @param distance The distance the light reaches.
     * @param shineColor The color that the light will use for specular
     * highlights.
     */
    Light(const Position3d &pos = {0.0f, 0.0f, 0.0f},
          const Color &color = Color::white(), float distance = 50.f,
          const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor),
          distance(distance) {}

    /**
     * @brief Function that sets the color of the light.
     *
     * @param color The new color for the light.
     */
    void setColor(Color color);

    /**
     * @brief Function that creates a debug obejct to visualize the light in the
     * scene.
     *
     */
    void createDebugObject();
    /**
     * @brief Function that adds the debug object to the window.
     *
     * @param window The window in which to add the debug object.
     */
    void addDebugObject(Window &window);

    /**
     * @brief The debug object that visualizes the light in the scene.
     *
     */
    std::shared_ptr<CoreObject> debugObject = nullptr;

    /**
     * @brief Function that calculates the constants for the point light.
     *
     * @return (PointLightConstants) The calculated constants.
     */
    PointLightConstants calculateConstants() const;

    /**
     * @brief Distance to which the light reaches.
     *
     */
    float distance = 50.f;

    /**
     * @brief Function that casts shadows from the light.
     *
     * @param window The window in which to cast shadows.
     * @param resolution The resolution from which to build the shadow map.
     */
    void castShadows(Window &window, int resolution = 1024);

    /**
     * @brief The render target that holds the shadow map.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;

  private:
    bool doesCastShadows = false;

    std::vector<glm::mat4> calculateShadowTransforms();

    friend class Window;
    friend class CoreObject;
};

/**
 * @brief Structure representing a directional light in a scene. A directional
 * light emits light in a specific direction, simulating sunlight.
 *
 * \subsection example-directional-light Example
 * ```cpp
 * // Create a directional light pointing downwards with white color
 * DirectionalLight dirLight({0.0f, -1.0f, 0.0f}, Color::white());
 * // Set the light color to a warm yellow
 * dirLight.setColor(Color(1.0f, 0.95f, 0.8f));
 * // Enable shadow casting for the light
 * dirLight.castShadows(window);
 * // Add the light to the scene
 * this->addDirectionalLight(&dirLight);
 * ```
 *
 */
class DirectionalLight {
  public:
    /**
     * @brief The direction in which the light is pointing. This should be a
     * normalized vector.
     *
     */
    Magnitude3d direction;

    /**
     * @brief The color of the light.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the light will use for specular highlights.
     *
     */
    Color shineColor = Color::white();

    /**
     * @brief Function that constructs a new DirectionalLight object.
     *
     * @param dir The direction in which the light is pointing.
     * @param color The color for the light.
     * @param shineColor The color that the light will use for specular
     * highlights.
     */
    DirectionalLight(const Magnitude3d &dir = {0.0f, -1.0f, 0.0f},
                     const Color &color = Color::white(),
                     const Color &shineColor = Color::white())
        : direction(dir.normalized()), color(color), shineColor(shineColor) {}

    /**
     * @brief Function that sets the color of the light.
     *
     * @param color The new color for the light.
     */
    void setColor(Color color);

    /**
     * @brief Object that holds the render target for shadow mapping.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;

    /**
     * @brief Function that enables casting shadows from the light.
     *
     * @param window  The window in which to cast shadows.
     * @param resolution The resolution to use for the shadow map.
     */
    void castShadows(Window &window, int resolution = 2048);

  private:
    bool doesCastShadows = false;

    ShadowParams
    calculateLightSpaceMatrix(std::vector<Renderable *> renderable) const;

    friend class Window;
    friend class CoreObject;
    friend class Terrain;

  private:
    std::vector<glm::vec4>
    getCameraFrustumCornersWorldSpace(Camera *camera, Window *window) const;
};

/**
 * @brief Structure representing a spotlight in a scene. A spotlight emits light
 * in a specific direction with a cone angle.
 * \subsection example-spotlight Example
 * ```cpp
 * // Create a spotlight at position (0, 10, 0) pointing downwards with white
 * color Spotlight spotLight({0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
 * Color::white(), 30.0f, 35.0f);
 * // Set the light color to a cool blue
 * spotLight.setColor(Color(0.7f, 0.8f, 1.0f));
 * // Enable shadow casting for the spotlight
 * spotLight.castShadows(window);
 * // Make the spotlight look at the origin
 * spotLight.lookAt({0.0f, 0.0f, 0.0f});
 * // Add a debug object to visualize the spotlight in the scene
 * spotLight.createDebugObject();
 * spotLight.addDebugObject(window);
 * // Add the spotlight to the scene
 * this->addSpotlight(&spotLight);
 * ```
 */
struct Spotlight {
    /**
     * @brief The position of the spotlight in 3D space.
     *
     */
    Position3d position = {0.0f, 0.0f, 0.0f};
    /**
     * @brief The direction in which the spotlight is pointing.
     *
     */
    Magnitude3d direction = {0.0f, -1.0f, 0.0f};

    /**
     * @brief The color of the spotlight.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the spotlight will use for specular highlights.
     *
     */
    Color shineColor = Color::white();

    /**
     * @brief Function that constructs a new Spotlight object.
     *
     * @param pos The position of the spotlight in 3D space.
     * @param dir The direction in which the spotlight is pointing.
     * @param color The color for the spotlight.
     * @param angle The inner cone angle of the spotlight in degrees.
     * @param outerAngle The outer cone angle of the spotlight in degrees.
     * @param shineColor The color that the spotlight will use for specular
     * highlights.
     */
    Spotlight(const Position3d &pos = {0.0f, 0.0f, 0.0f},
              Magnitude3d dir = {0.0f, -1.0f, 0.0f},
              const Color &color = Color::white(), const float angle = 35.f,
              const float outerAngle = 40.f,
              const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor), direction(dir),
          cutOff(glm::cos(glm::radians(static_cast<double>(angle)))),
          outerCutoff(glm::cos(glm::radians(static_cast<double>(outerAngle)))) {
    }

    /**
     * @brief Function that sets the color of the spotlight.
     *
     * @param color The new color for the spotlight.
     */
    void setColor(Color color);

    /**
     * @brief Function that creates a debug object to visualize the spotlight in
     * the scene.
     *
     */
    void createDebugObject();
    /**
     * @brief Function that adds the debug object to the window.
     *
     * @param window The window in which to add the debug object.
     */
    void addDebugObject(Window &window);
    /**
     * @brief Function that updates the rotation of the debug object.
     *
     */
    void updateDebugObjectRotation();
    /**
     * @brief Function that makes the spotlight look at a target position.
     *
     * @param target The position to look at.
     */
    void lookAt(const Position3d &target);

    /**
     * @brief The debug object that visualizes the spotlight in the scene.
     *
     */
    std::shared_ptr<CoreObject> debugObject = nullptr;

    /**
     * @brief The inner cone angle of the spotlight in degrees.
     *
     */
    float cutOff;
    /**
     * @brief The outer cone angle of the spotlight in degrees.
     *
     */
    float outerCutoff;

    /**
     * @brief The render target to which the spotlight casts shadows.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;

    /**
     * @brief Function that enables casting shadows from the spotlight.
     *
     * @param window The window in which to cast shadows.
     * @param resolution The resolution to use for the shadow map.
     */
    void castShadows(Window &window, int resolution = 1024);

  private:
    bool doesCastShadows = true;

    std::tuple<glm::mat4, glm::mat4> calculateLightSpaceMatrix() const;

    friend class Window;
    friend class CoreObject;
};

#endif // ATLAS_LIGHT_H
