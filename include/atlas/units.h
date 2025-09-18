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

#include <glm/glm.hpp>

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

    inline glm::vec3 toGlm() const {
        return glm::vec3(static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(z));
    }
};

typedef Position3d Scale3d;

struct Rotation3d {
    double pitch; // Rotation around the X-axis
    double yaw;   // Rotation around the Y-axis
    double roll;  // Rotation around the Z-axis

    Rotation3d operator+(const Rotation3d &other) const {
        return {pitch + other.pitch, yaw + other.yaw, roll + other.roll};
    }

    Rotation3d operator-(const Rotation3d &other) const {
        return {pitch - other.pitch, yaw - other.yaw, roll - other.roll};
    }

    Rotation3d operator*(double scalar) const {
        return {pitch * scalar, yaw * scalar, roll * scalar};
    }

    Rotation3d operator/(double scalar) const {
        return {pitch / scalar, yaw / scalar, roll / scalar};
    }

    inline glm::vec3 toGlm() const {
        return glm::vec3(static_cast<float>(pitch), static_cast<float>(yaw),
                         static_cast<float>(roll));
    }
};

struct Color {
    double r;
    double g;
    double b;
    double a;

    Color operator+(const Color &other) const {
        return {r + other.r, g + other.g, b + other.b, a + other.a};
    }

    Color operator-(const Color &other) const {
        return {r - other.r, g - other.g, b - other.b, a - other.a};
    }

    Color operator*(double scalar) const {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    Color operator/(double scalar) const {
        return {r / scalar, g / scalar, b / scalar, a / scalar};
    }

    static Color white() { return {1.0, 1.0, 1.0, 1.0}; }
    static Color black() { return {0.0, 0.0, 0.0, 1.0}; }
    static Color red() { return {1.0, 0.0, 0.0, 1.0}; }
    static Color green() { return {0.0, 1.0, 0.0, 1.0}; }
    static Color blue() { return {0.0, 0.0, 1.0, 1.0}; }
    static Color transparent() { return {0.0, 0.0, 0.0, 0.0}; }
    static Color yellow() { return {1.0, 1.0, 0.0, 1.0}; }
    static Color cyan() { return {0.0, 1.0, 1.0, 1.0}; }
    static Color magenta() { return {1.0, 0.0, 1.0, 1.0}; }
    static Color gray() { return {0.5, 0.5, 0.5, 1.0}; }
    static Color orange() { return {1.0, 0.65, 0.0, 1.0}; }
    static Color purple() { return {0.5, 0.0, 0.5, 1.0}; }
    static Color brown() { return {0.6, 0.4, 0.2, 1.0}; }
    static Color pink() { return {1.0, 0.75, 0.8, 1.0}; }
    static Color lime() { return {0.0, 1.0, 0.0, 1.0}; }
    static Color navy() { return {0.0, 0.0, 0.5, 1.0}; }
    static Color teal() { return {0.0, 0.5, 0.5, 1.0}; }
    static Color olive() { return {0.5, 0.5, 0.0, 1.0}; }
    static Color maroon() { return {0.5, 0.0, 0.0, 1.0}; }

    inline glm::vec4 toGlm() const {
        return glm::vec4(static_cast<float>(r), static_cast<float>(g),
                         static_cast<float>(b), static_cast<float>(a));
    }
};

typedef unsigned int Id;

#endif // ATLAS_UNITS_H
