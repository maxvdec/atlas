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

#include "atlas/object.h"
#include "atlas/units.h"
#include <memory>

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
};

#endif // ATLAS_LIGHT_H
