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
#include <stdexcept>

struct Position2d {
    float x;
    float y;

    Position2d(float x = 0, float y = 0) : x(x), y(y) {}
};

enum class Axis { X, Y, Z };

struct Position3d {
    float x;
    float y;
    float z;

    Position3d(float x = 0.0f, float y = 0.0f, float z = 0.0f)
        : x(x), y(y), z(z) {}

    inline bool operator==(const Position3d &other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    inline bool operator!=(const Position3d &other) const {
        return !(*this == other);
    }

    inline Position3d operator+(const Position3d &other) const {
        return Position3d(x + other.x, y + other.y, z + other.z);
    }

    inline Position3d operator-(const Position3d &other) const {
        return Position3d(x - other.x, y - other.y, z - other.z);
    }

    inline Position3d operator*(float scalar) const {
        return Position3d(x * scalar, y * scalar, z * scalar);
    }

    inline Position3d operator*(const glm::vec3 &vec) const {
        return Position3d(x * vec.x, y * vec.y, z * vec.z);
    }

    inline Position3d operator/(float scalar) const {
        if (scalar == 0.0f) {
            throw std::invalid_argument("Division by zero in Position3d");
        }
        return Position3d(x / scalar, y / scalar, z / scalar);
    }

    inline Position3d operator-() const { return Position3d(-x, -y, -z); }

    inline void operator+=(const Position3d &other) {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    inline void operator-=(const Position3d &other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }

    inline void operator+=(const glm::vec3 &vec) {
        x += vec.x;
        y += vec.y;
        z += vec.z;
    }

    inline void operator-=(const glm::vec3 &vec) {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
    }

    inline glm::vec3 toVec3() const { return glm::vec3(x, y, z); }

    inline Position3d withInverted(Axis axis) const {
        switch (axis) {
        case Axis::X:
            return Position3d(-x, y, z);
        case Axis::Y:
            return Position3d(x, -y, z);
        case Axis::Z:
            return Position3d(x, y, -z);
        default:
            throw std::invalid_argument("Invalid axis for inversion");
        }
    }
};

struct Size2d {
    float width;
    float height;

    Size2d(float width = 0, float height = 0) : width(width), height(height) {}
    Size2d(int width, int height)
        : width(static_cast<float>(width)), height(static_cast<float>(height)) {
    }
};

struct Size3d {
    float width;
    float height;
    float depth;

    Size3d(float width = 0, float height = 0, float depth = 0)
        : width(width), height(height), depth(depth) {}
    Size3d(int width, int height, int depth)
        : width(static_cast<float>(width)), height(static_cast<float>(height)),
          depth(static_cast<float>(depth)) {}
};

struct Color {
    float r;
    float g;
    float b;
    float a;

    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}

    inline glm::vec3 toVec3() const { return glm::vec3(r, g, b); }
    inline glm::vec4 toVec4() const { return glm::vec4(r, g, b, a); }
};

typedef Size2d Frame;

#endif // ATLAS_UNITS_HPP
