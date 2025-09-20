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

struct Light {
    Position3d position = {0.0f, 0.0f, 0.0f};

    Color color = Color::white();
    Color shineColor = Color::white();

    Light(const Position3d &pos = {0.0f, 0.0f, 0.0f},
          const Color &color = Color::white(),
          const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor) {}

    static std::shared_ptr<Light>
    create(const Position3d &pos = {0.0f, 0.0f, 0.0f},
           const Color &color = Color::white(),
           const Color &shineColor = Color::white()) {
        return std::make_shared<Light>(pos, color, shineColor);
    }

    void setColor(Color color);

    void createDebugObject();
    void addDebugObject(Window &window);

    std::shared_ptr<CoreObject> debugObject = nullptr;
};

#endif // ATLAS_LIGHT_H
