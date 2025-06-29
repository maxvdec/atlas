/*
 light.cpp - FIXED VERSION
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting functions and solutions (FIXED)
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.hpp"
#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/units.hpp"
#include <glm/glm.hpp>
#include <iostream>

Light::Light(Position3d position, Color color)
    : position(position), color(color) {

    this->debugObject = generateCubeObject(position, Size3d(0.1f, 0.1f, 0.1f));

    for (int i = 0; i < debugObject.vertices.size(); ++i) {
        debugObject.setVertexColor(i, color);
        std::cout << "Setting vertex color for debug object at index " << i
                  << " to " << color.r << ", " << color.g << ", " << color.b
                  << ", " << color.a << std::endl;
    }

    debugObject.translate(position.x, position.y, position.z);

    debugObject.initialize();
    debugObject.disableTexturing();
}

void Light::debugLight() { this->debugObject.show(); }
