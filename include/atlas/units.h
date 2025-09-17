/*
 units.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Unit definition and shorthand expression
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_UNITS_H
#define ATLAS_UNITS_H

#ifdef ATLAS_LIBRARY_IMPL
#include <glm/glm.hpp>
#endif

struct Position3d {
    double x;
    double y;
    double z;

    Position3d operator+(const Position3d &other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Position3d operator-(const Position3d &other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Position3d operator*(double scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    Position3d operator/(double scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

#ifdef ATLAS_LIBRARY_IMPL
    inline glm::vec3 toGlm() const {
        return glm::vec3(static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(z));
    }
#endif
};

typedef unsigned int Id;

#endif // ATLAS_UNITS_H
