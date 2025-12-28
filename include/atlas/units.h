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
#include <cmath>
#include <numbers>
#include <ostream>

/**
 * @brief Structure representing a position in 3D space with double precision.
 * Provides arithmetic operations and conversions to/from GLM types.
 *
 * \subsection position3d-example Example
 * ```cpp
 * // Create a position
 * Position3d pos(10.0f, 5.0f, -3.0f);
 * // Move it
 * pos += Position3d(1.0f, 0.0f, 0.0f);
 * // Convert to GLM vector for rendering
 * glm::vec3 glmPos = pos.toGlm();
 * ```
 */
struct Position3d {
    float x;
    float y;
    float z;

    Position3d() : x(0.0f), y(0.0f), z(0.0f) {}
    Position3d(float x, float y, float z) : x(x), y(y), z(z) {}
    Position3d(double x, double y, double z)
        : x(static_cast<float>(x)), y(static_cast<float>(y)),
          z(static_cast<float>(z)) {}

    Position3d operator+(const Position3d &other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Position3d operator-(const Position3d &other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Position3d operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    Position3d operator/(float scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

    bool operator==(const Position3d &other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    inline glm::vec3 toGlm() const { return glm::vec3(x, y, z); }

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
        float length = std::sqrt(x * x + y * y + z * z);
        if (length == 0)
            return {};
        return {x / length, y / length, z / length};
    }

    inline static Position3d fromGlm(const glm::vec3 &vec) {
        return {vec.x, vec.y, vec.z};
    };

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
 * \subsection rotation3d-example Example
 * ```cpp
 * // Create a rotation (pitch, yaw, roll in degrees)
 * Rotation3d rot(0.0f, 45.0f, 0.0f);
 * // Rotate object over time
 * rot.yaw += deltaTime * 90.0f; // 90 degrees per second
 * // Convert to quaternion for rendering
 * glm::quat quatRot = rot.toGlmQuat();
 * ```
 */
struct Rotation3d {
    float pitch; // Rotation around the X-axis
    float yaw;   // Rotation around the Y-axis
    float roll;  // Rotation around the Z-axis

    Rotation3d() : pitch(0.0f), yaw(0.0f), roll(0.0f) {}

    Rotation3d(float pitch, float yaw, float roll)
        : pitch(pitch), yaw(yaw), roll(roll) {}

    Rotation3d(double pitch, double yaw, double roll)
        : pitch(static_cast<float>(pitch)), yaw(static_cast<float>(yaw)),
          roll(static_cast<float>(roll)) {}

    Rotation3d operator+(const Rotation3d &other) const {
        return {pitch + other.pitch, yaw + other.yaw, roll + other.roll};
    }

    Rotation3d operator-(const Rotation3d &other) const {
        return {pitch - other.pitch, yaw - other.yaw, roll - other.roll};
    }

    Rotation3d operator*(float scalar) const {
        return {pitch * scalar, yaw * scalar, roll * scalar};
    }

    bool operator==(const Rotation3d &other) const {
        return pitch == other.pitch && yaw == other.yaw && roll == other.roll;
    }

    Rotation3d operator/(float scalar) const {
        return {pitch / scalar, yaw / scalar, roll / scalar};
    }

    inline glm::vec3 toGlm() const { return glm::vec3(pitch, yaw, roll); }

    inline glm::quat toGlmQuat() const {
        const float pitchRad = glm::radians(pitch);
        const float yawRad = glm::radians(yaw);
        const float rollRad = glm::radians(roll);

        const glm::quat qYaw = glm::angleAxis(yawRad, glm::vec3(0, 1, 0));
        const glm::quat qPitch = glm::angleAxis(pitchRad, glm::vec3(1, 0, 0));
        const glm::quat qRoll = glm::angleAxis(rollRad, glm::vec3(0, 0, 1));

        return qRoll * qPitch * qYaw;
    }

    inline static Rotation3d fromGlmQuat(const glm::quat &quat) {
        glm::mat3 m = glm::mat3_cast(quat);

        float sPitch = glm::clamp(m[1][2], -1.0f, 1.0f);
        float pitchRad = std::asin(sPitch);
        float cPitch = std::cos(pitchRad);

        float yawRad;
        float rollRad;

        constexpr float eps = 1e-6f;
        if (std::abs(cPitch) > eps) {
            yawRad = std::atan2(-m[0][2], m[2][2]);
            rollRad = std::atan2(-m[1][0], m[1][1]);
        } else {
            yawRad = std::atan2(sPitch * m[0][1], m[0][0]);
            rollRad = 0.0f;
        }

        return {glm::degrees(pitchRad), glm::degrees(yawRad),
                glm::degrees(rollRad)};
    }

    inline static Rotation3d fromGlm(const glm::vec3 &vec) {
        return {vec.x, vec.y, vec.z};
    }
};

/**
 * @brief Structure representing an RGBA color with double precision.
 * Provides arithmetic operations and common color constants.
 *
 * \subsection color-example Example
 * ```cpp
 * // Create colors using static constructors
 * Color red = Color::Red();
 * Color customColor(0.2, 0.8, 0.5, 1.0);
 * // Parse from hex string
 * Color fromHex = Color::fromHex("#FF5733");
 * // Mix two colors
 * Color blended = Color::mix(red, customColor, 0.5);
 * // Convert to GLM vector
 * glm::vec4 glmColor = blended.toGlm();
 * ```
 */
struct Color {
    float r = 1.0;
    float g = 1.0;
    float b = 1.0;
    float a = 1.0;

    Color operator+(const Color &other) const {
        return {r + other.r, g + other.g, b + other.b, a + other.a};
    }

    Color operator-(const Color &other) const {
        return {r - other.r, g - other.g, b - other.b, a - other.a};
    }

    Color operator*(float scalar) const {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    Color operator*(const Color &other) const {
        return {r * other.r, g * other.g, b * other.b, a * other.a};
    }

    Color operator/(float scalar) const {
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
    static Color orange() { return {1.0, 0.65f, 0.0, 1.0}; }
    static Color purple() { return {0.5, 0.0, 0.5, 1.0}; }
    static Color brown() { return {0.6f, 0.4f, 0.2f, 1.0}; }
    static Color pink() { return {1.0, 0.75f, 0.8f, 1.0}; }
    static Color lime() { return {0.0, 1.0, 0.0, 1.0}; }
    static Color navy() { return {0.0, 0.0, 0.5, 1.0}; }
    static Color teal() { return {0.0, 0.5, 0.5, 1.0}; }
    static Color olive() { return {0.5, 0.5, 0.0, 1.0}; }
    static Color maroon() { return {0.5, 0.0, 0.0, 1.0}; }

    static Color fromHex(unsigned int hexValue) {
        float r = static_cast<float>(((hexValue >> 16) & 0xFF) / 255.0);
        float g = static_cast<float>(((hexValue >> 8) & 0xFF) / 255.0);
        float b = static_cast<float>((hexValue & 0xFF) / 255.0);
        return {r, g, b, 1.0f};
    }

    static Color mix(Color color1, Color color2, float ratio = 0.5) {
        float r = color1.r * (1 - ratio) + color2.r * ratio;
        float g = color1.g * (1 - ratio) + color2.g * ratio;
        float b = color1.b * (1 - ratio) + color2.b * ratio;
        float a = color1.a * (1 - ratio) + color2.a * ratio;
        return {r, g, b, a};
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
enum class Direction3d {
    /**
     * @brief Positive Y axis.
     */
    Up,
    /**
     * @brief Negative Y axis.
     */
    Down,
    /**
     * @brief Negative X axis.
     */
    Left,
    /**
     * @brief Positive X axis.
     */
    Right,
    /**
     * @brief Positive Z axis.
     */
    Forward,
    /**
     * @brief Negative Z axis.
     */
    Backward
};

/**
 * @brief Structure representing a position in 2D space with double precision.
 * Provides arithmetic operations and conversions to GLM types.
 *
 */
struct Position2d {
    float x;
    float y;

    Position2d operator+(const Position2d &other) const {
        return {x + other.x, y + other.y};
    }

    Position2d operator-(const Position2d &other) const {
        return {x - other.x, y - other.y};
    }

    Position2d operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    Position2d operator/(float scalar) const {
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
typedef Position2d Magnitude2d;

/**
 * @brief Structure representing angular measurements in radians.
 * Provides arithmetic operations and conversion from degrees.
 *
 * \subsection radians-example Example
 * ```cpp
 * // Convert from degrees to radians
 * Radians angle = Radians::fromDegrees(45.0f);
 * // Perform angle arithmetic
 * Radians doubled = angle * 2.0f;
 * // Convert back to float for trigonometry
 * float sinValue = std::sin(angle.toFloat());
 * ```
 */
struct Radians {
    float value;

    Radians operator+(const Radians &other) const {
        return {value + other.value};
    }

    Radians operator-(const Radians &other) const {
        return {value - other.value};
    }

    Radians operator*(float scalar) const { return {value * scalar}; }

    Radians operator/(float scalar) const { return {value / scalar}; }

    inline float toFloat() const { return static_cast<float>(value); }

    inline static Radians fromDegrees(float degrees) {
        return {static_cast<float>(degrees * (std::numbers::pi) / 180.0f)};
    }
};

/**
 * @brief Structure representing 2D dimensions with width and height.
 * Provides arithmetic operations and conversions to GLM types.
 *
 * \subsection size2d-example Example
 * ```cpp
 * // Create window or UI element size
 * Size2d windowSize{1920.0f, 1080.0f};
 * // Scale size
 * Size2d halfSize = windowSize / 2.0f;
 * // Calculate area
 * float area = windowSize.width * windowSize.height;
 * // Convert to GLM vector
 * glm::vec2 glmSize = windowSize.toGlm();
 * ```
 */
struct Size2d {
    float width;
    float height;

    Size2d operator+(const Size2d &other) const {
        return {width + other.width, height + other.height};
    }

    Size2d operator-(const Size2d &other) const {
        return {width - other.width, height - other.height};
    }

    Size2d operator*(float scalar) const {
        return {width * scalar, height * scalar};
    }

    Size2d operator/(float scalar) const {
        return {width / scalar, height / scalar};
    }

    inline glm::vec2 toGlm() const {
        return glm::vec2(static_cast<float>(width), static_cast<float>(height));
    }
};

#endif // ATLAS_UNITS_H
