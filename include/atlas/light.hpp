/*
 light.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Light header and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_LIGHT_HPP
#define ATLAS_LIGHT_HPP

#include "atlas/core/rendering.hpp"
#include "atlas/material.hpp"
#include "atlas/units.hpp"
#include <glm/glm.hpp>

class Light {
  public:
    Position3d position;
    Color color;
    Color ambientColor = Color(0.2f, 0.2f, 0.2f, 1.0f);

    Light(Position3d position = Position3d(0.0f, 0.0f, 0.0f),
          Color color = Color(1.0f, 1.0f, 1.0f));

    void debugLight();
    CoreObject debugObject;
    float intensity = 0.5f;

    Material material = Material();
};

#endif // ATLAS_LIGHT_HPP
