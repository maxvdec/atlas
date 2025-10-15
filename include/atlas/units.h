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
#include <glm/gtc/quaternion.hpp>
#include <numbers>
#include <ostream>

/**
 * @brief Structure representing a position in 3D space with double precision.
 * Provides arithmetic operations and conversions to/from GLM types.
 *
 */
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

    bool operator==(const Position3d &other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    inline glm::vec3 toGlm() const {
        return glm::vec3(static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(z));
    }

    Position3d operator+(const glm::vec3 &vec) const {
        return {x + vec.x, y + vec.y, z + vec.z};
    }

    Position3d operator-(const glm::vec3 &vec) const {
        return {x - vec.x, y - vec.y, z - vec.z};
    }

    void operator+=(const Position3d &vec) {
        x += vec.x;
        y += vec.y;
        z += vec.z;
    }

    void operator-=(const Position3d &vec) {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
    }

    inline Position3d normalized() const {
        double length = std::sqrt(x * x + y * y + z * z);
        if (length == 0)
            return {0, 0, 0};
        return {x / length, y / length, z / length};
    }

    inline static Position3d fromGlm(const glm::vec3 &vec) {
        return {static_cast<double>(vec.x), static_cast<double>(vec.y),
                static_cast<double>(vec.z)};
    }

    inline friend std::ostream &operator<<(std::ostream &os,
                                           const Position3d &p) {
        os << "Position3d(" << p.x << ", " << p.y << ", " << p.z << ")";
        return os;
    }
};

/**
 * @brief Type alias for 3D scaling factors.
 *
 */
typedef Position3d Scale3d;
/**
 * @brief Type alias for 3D size dimensions.
 *
 */
typedef Position3d Size3d;
/**
 * @brief Type alias for 3D points.
 *
 */
typedef Position3d Point3d;
/**
 * @brief Type alias for 3D normal vectors.
 *
 */
typedef Position3d Normal3d;
/**
 * @brief Type alias for 3D magnitude vectors.
 *
 */
typedef Position3d Magnitude3d;

/**
 * @brief Structure representing rotation in 3D space using Euler angles.
 * Provides arithmetic operations and conversions to/from GLM quaternions.
 *
 */
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

    bool operator==(const Rotation3d &other) const {
        return pitch == other.pitch && yaw == other.yaw && roll == other.roll;
    }

    Rotation3d operator/(double scalar) const {
        return {pitch / scalar, yaw / scalar, roll / scalar};
    }

    inline glm::vec3 toGlm() const {
        return glm::vec3(static_cast<float>(pitch), static_cast<float>(yaw),
                         static_cast<float>(roll));
    }

    inline glm::quat toGlmQuat() const {
        glm::vec3 eulerAngles = toGlm();
        glm::vec3 radians = glm::radians(eulerAngles);
        return glm::quat(radians);
    }

    inline static Rotation3d fromGlmQuat(const glm::quat &quat) {
        glm::vec3 euler = glm::degrees(glm::eulerAngles(quat));
        return {static_cast<double>(euler.x), static_cast<double>(euler.y),
                static_cast<double>(euler.z)};
    }

    inline static Rotation3d fromGlm(const glm::vec3 &vec) {
        return {static_cast<double>(vec.x), static_cast<double>(vec.y),
                static_cast<double>(vec.z)};
    }
};

/**
 * @brief Structure representing an RGBA color with double precision.
 * Provides arithmetic operations and common color constants.
 *
 */
struct Color {
    double r = 1.0;
    double g = 1.0;
    double b = 1.0;
    double a = 1.0;

    Color operator+(const Color &other) const {
        return {r + other.r, g + other.g, b + other.b, a + other.a};
    }

    Color operator-(const Color &other) const {
        return {r - other.r, g - other.g, b - other.b, a - other.a};
    }

    Color operator*(double scalar) const {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    Color operator*(const Color &other) const {
        return {r * other.r, g * other.g, b * other.b, a * other.a};
    }

    Color operator/(double scalar) const {
        return {r / scalar, g / scalar, b / scalar, a / scalar};
    }

    bool operator==(const Color &other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
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

    static Color fromHex(unsigned int hexValue) {
        double r = ((hexValue >> 16) & 0xFF) / 255.0;
        double g = ((hexValue >> 8) & 0xFF) / 255.0;
        double b = (hexValue & 0xFF) / 255.0;
        return {r, g, b, 1.0};
    }

    inline glm::vec4 toGlm() const {
        return glm::vec4(static_cast<float>(r), static_cast<float>(g),
                         static_cast<float>(b), static_cast<float>(a));
    }
};

/**
 * @brief Type alias for OpenGL object identifiers.
 *
 */
typedef unsigned int Id;

/**
 * @brief Enumeration of 3D directional constants.
 *
 */
enum class Direction3d { Up, Down, Left, Right, Forward, Backward };

/**
 * @brief Structure representing a position in 2D space with double precision.
 * Provides arithmetic operations and conversions to GLM types.
 *
 */
struct Position2d {
    double x;
    double y;

    Position2d operator+(const Position2d &other) const {
        return {x + other.x, y + other.y};
    }

    Position2d operator-(const Position2d &other) const {
        return {x - other.x, y - other.y};
    }

    Position2d operator*(double scalar) const {
        return {x * scalar, y * scalar};
    }

    Position2d operator/(double scalar) const {
        return {x / scalar, y / scalar};
    }

    inline glm::vec2 toGlm() const {
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }
};

/**
 * @brief Type alias for 2D scaling factors.
 *
 */
typedef Position2d Scale2d;
/**
 * @brief Type alias for 2D points.
 *
 */
typedef Position2d Point2d;
/**
 * @brief Type alias for 2D movement vectors.
 *
 */
typedef Position2d Movement2d;

/**
 * @brief Structure representing angular measurements in radians.
 * Provides arithmetic operations and conversion from degrees.
 *
 */
struct Radians {
    double value;

    Radians operator+(const Radians &other) const {
        return {value + other.value};
    }

    Radians operator-(const Radians &other) const {
        return {value - other.value};
    }

    Radians operator*(double scalar) const { return {value * scalar}; }

    Radians operator/(double scalar) const { return {value / scalar}; }

    inline float toFloat() const { return static_cast<float>(value); }

    inline static Radians fromDegrees(double degrees) {
        return {degrees * (std::numbers::pi) / 180.0};
    }
};

/**
 * @brief Structure representing 2D dimensions with width and height.
 * Provides arithmetic operations and conversions to GLM types.
 *
 */
struct Size2d {
    double width;
    double height;

    Size2d operator+(const Size2d &other) const {
        return {width + other.width, height + other.height};
    }

    Size2d operator-(const Size2d &other) const {
        return {width - other.width, height - other.height};
    }

    Size2d operator*(double scalar) const {
        return {width * scalar, height * scalar};
    }

    Size2d operator/(double scalar) const {
        return {width / scalar, height / scalar};
    }

    inline glm::vec2 toGlm() const {
        return glm::vec2(static_cast<float>(width), static_cast<float>(height));
    }
};

#endif // ATLAS_UNITS_H
