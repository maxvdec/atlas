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

class Window;

struct AmbientLight {
    Color color;
    float intensity;
};

struct Light {
    Position3d position;

    Color color;
    Color shineColor = Color::white();

    Light(const Position3d &pos, const Color &color = Color::white(),
          const Color &shineColor = Color::white())
        : position(pos), color(color), shineColor(shineColor) {}

    CoreObject createDebugObject();
};

#endif // ATLAS_LIGHT_H
