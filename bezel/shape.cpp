/*
 shape.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shape functions
 Copyright (c) 2025 maxvdec
*/

#include "bezel/shape.h"

Sphere::Sphere(float radius) : radius(radius) {
    this->centerOfMass = {0.0f, 0.0f, 0.0f};
}
