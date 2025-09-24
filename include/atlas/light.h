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
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>

class Window;

struct AmbientLight {
    Color color;
    float intensity;
};

struct PointLightConstants {
    float distance;
    float constant;
    float linear;
    float quadratic;
};

struct ShadowParams {
    glm::mat4 lightView;
    glm::mat4 lightProjection;
    float bias;
};

struct Light {
    Position3d position = {0.0f, 0.0f, 0.0f};

    Color color = Color::white();
    Color shineColor = Color::white();

    Light(const Position3d &pos = {0.0f, 0.0f, 0.0f},
          const Color &color = Color::white(), float distance = 50.f,
          const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor),
          distance(distance) {}

    void setColor(Color color);

    void createDebugObject();
    void addDebugObject(Window &window);

    std::shared_ptr<CoreObject> debugObject = nullptr;

    PointLightConstants calculateConstants() const;

    float distance = 50.f;

    void castShadows(Window &window, int resolution = 1024);

    RenderTarget *shadowRenderTarget = nullptr;

  private:
    bool doesCastShadows = false;

    std::vector<glm::mat4> calculateShadowTransforms();

    friend class Window;
    friend class CoreObject;
};

class DirectionalLight {
  public:
    Magnitude3d direction;

    Color color = Color::white();
    Color shineColor = Color::white();

    DirectionalLight(const Magnitude3d &dir = {0.0f, -1.0f, 0.0f},
                     const Color &color = Color::white(),
                     const Color &shineColor = Color::white())
        : direction(dir.normalized()), color(color), shineColor(shineColor) {}

    void setColor(Color color);

    RenderTarget *shadowRenderTarget = nullptr;

    void castShadows(Window &window, int resolution = 2048);

  private:
    bool doesCastShadows = false;

    ShadowParams
    calculateLightSpaceMatrix(std::vector<Renderable *> renderable) const;

    friend class Window;
    friend class CoreObject;

  private:
    std::vector<glm::vec4>
    getCameraFrustumCornersWorldSpace(Camera *camera, Window *window) const;
};

struct Spotlight {
    Position3d position = {0.0f, 0.0f, 0.0f};
    Magnitude3d direction = {0.0f, -1.0f, 0.0f};

    Color color = Color::white();
    Color shineColor = Color::white();

    Spotlight(const Position3d &pos = {0.0f, 0.0f, 0.0f},
              Magnitude3d dir = {0.0f, -1.0f, 0.0f},
              const Color &color = Color::white(), const float angle = 35.f,
              const float outerAngle = 40.f,
              const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor), direction(dir),
          cutOff(glm::cos(glm::radians(static_cast<double>(angle)))),
          outerCutoff(glm::cos(glm::radians(static_cast<double>(outerAngle)))) {
    }

    void setColor(Color color);

    void createDebugObject();
    void addDebugObject(Window &window);
    void updateDebugObjectRotation();
    void lookAt(const Position3d &target);

    std::shared_ptr<CoreObject> debugObject = nullptr;

    float cutOff;
    float outerCutoff;

    RenderTarget *shadowRenderTarget = nullptr;

    void castShadows(Window &window, int resolution = 1024);

  private:
    bool doesCastShadows = true;

    std::tuple<glm::mat4, glm::mat4> calculateLightSpaceMatrix() const;

    friend class Window;
    friend class CoreObject;
};

#endif // ATLAS_LIGHT_H
