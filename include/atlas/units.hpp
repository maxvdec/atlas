/*
 units.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Units and utility structs
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_UNITS_HPP
#define ATLAS_UNITS_HPP

#include <glm/glm.hpp>

struct Position2d {
    int x;
    int y;

    Position2d(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Position3d {
    float x;
    float y;
    float z;

    Position3d(float x = 0.0f, float y = 0.0f, float z = 0.0f)
        : x(x), y(y), z(z) {}

    inline bool operator==(const Position3d &other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    inline glm::vec3 toVec3() const { return glm::vec3(x, y, z); }
};

struct Size2d {
    int width;
    int height;

    Size2d(int width = 0, int height = 0) : width(width), height(height) {}
};

struct Color {
    float r;
    float g;
    float b;
    float a;

    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
};

enum class Axis { X, Y, Z };

typedef Size2d Frame;

#endif // ATLAS_UNITS_HPP
