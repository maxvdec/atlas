/*
 material.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Material functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_MATERIAL_HPP
#define ATLAS_MATERIAL_HPP

#include "atlas/units.hpp"
#define SHININESS_NONE 2.0f
#define SHININESS_VERY_LOW 8.0f
#define SHININESS_LOW 16.0f
#define SHININESS_MEDIUM 32.0f
#define SHININESS_HIGH 64.0f
#define SHININESS_VERY_HIGH 128.0f
#define SHININESS_EXTREME 256.0f

struct Material {
    float shininess = SHININESS_MEDIUM;
    Color ambientColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    Color diffuse = Color(0.8f, 0.8f, 0.8f, 1.0f);
    Color specular = Color(1.0f, 1.0f, 1.0f, 1.0f);

    inline void setReflection(float reflection) {
        if (reflection < 0.0f || reflection > 1.0f) {
            throw std::invalid_argument(
                "Reflection must be between 0.0 and 1.0");
        }
        this->specular = Color(reflection, reflection, reflection, 1.0f);
    }
};

#endif // ATLAS_MATERIAL_HPP
