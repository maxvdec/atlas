/*
 camera.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Camera definition and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef CAMERA_H
#define CAMERA_H

#include "atlas/units.h"
#include <glm/glm.hpp>

class Window;

/**
 * @brief Class representing a camera in 3D space, capable of generating view
 * and projection matrices. It also has built-in controls for moving and looking
 * with native support for both perspective and orthographic projections.
 *
 * \subsection example-camera-control Example of camera control
 * ```cpp
 * // Create a camera instance
 * Camera camera;
 * // Set the camera position
 * camera.setPosition({0.0f, 0.0f, 5.0f});
 * // Make the camera look at the origin
 * camera.lookAt({0.0f, 0.0f, 0.0f});
 * // On each frame, update the camera with input from the window
 * camera.update(window);
 * // On each mouse movement, update the look direction
 * camera.updateLook(window, mouseMovement);
 * // On each scroll event, update the zoom level (optional)
 * camera.updateZoom(window, scrollOffset);
 * ```
 *
 */
class Camera {
  public:
    /**
     * @brief The position of the camera in 3D space.
     *
     */
    Position3d position;
    /**
     * @brief The point where the camera is looking at in 3D space.
     *
     */
    Point3d target;

    /**
     * @brief The field of view of the camera.
     *
     */
    float fov = 45.0f; // Field of view in degrees
    /**
     * @brief The near clipping plane distance. Everything closer than this
     * value will be clipped.
     *
     */
    float nearClip = 0.1f;
    /**
     * @brief The far clipping plane distance. Everything farther than this
     * value will be clipped.
     *
     */
    float farClip = 100.0f;
    /**
     * @brief The size of the orthographic projection.
     *
     */
    float orthographicSize = 5.0f;

    /**
     * @brief The speed at which the camera moves through the scene.
     *
     */
    float movementSpeed = 2.f;
    /**
     * @brief The factor by which the camera's mouse input is scaled.
     *
     */
    float mouseSensitivity = 0.1f;
    /**
     * @brief The factor to determine how smoothly the camera looks at the
     * target.
     *
     */
    float lookSmoothness = 0.15f;

    /**
     * @brief Whether the camera is using orthographic projection.
     *
     */
    bool useOrthographic = false;

    /**
     * @brief The depth value at which objects are in perfect focus for depth
     * of field effects.
     *
     */
    float focusDepth = 20.f;
    /**
     * @brief The range around focusDepth where objects gradually transition
     * from sharp to blurred.
     *
     */
    float focusRange = 10.0f;

    /**
     * @brief Move the camera by a delta in 3D space.
     *
     * @param delta The delta by which to move the camera.
     */
    void move(const Position3d &delta);
    /**
     * @brief Sets the position of the camera in 3D space.
     *
     * @param newPosition The new position to set the camera to.
     */
    void setPosition(const Position3d &newPosition);
    /**
     * @brief Sets the point where the camera is looking at in 3D space.
     *
     * @param newTarget The new target point to look at.
     */
    void lookAt(const Point3d &newTarget);

    /**
     * @brief Construct a new Camera object
     *
     */
    Camera() : position({0.0f, 0.0f, 3.0f}), target({0.0f, 0.0f, 0.0f}) {}

    /**
     * @brief Updates the camera's position and orientation based on user input.
     *
     * @param window The window from which to get input.
     */
    void update(Window &window);
    /**
     * @brief Updates the camera's look direction based on mouse movement.
     *
     * @param window The window from which to get input.
     * @param movement The movement of the mouse.
     */
    void updateLook(Window &window, Movement2d movement);
    /**
     * @brief Updates the camera's zoom level based on scroll input.
     *
     * @param window The window from which to get input.
     * @param offset The offset of the scroll.
     */
    void updateZoom(Window &window, Movement2d offset);

    /**
     * @brief Moves the camera in a specified direction at a given speed.
     *
     * @param direction The direction from which to move the camera.
     * @param speed The speed at which to move the camera.
     */
    void moveTo(Direction3d direction, float speed);

    /**
     * @brief Calculates and returns the view matrix based on the camera's
     * position
     *
     * @return (glm::mat4) The calculated view matrix.
     */
    glm::mat4 calculateViewMatrix() const;

    /**
     * @brief Get the front vector of the camera, representing the direction it
     * is facing.
     *
     * @return (Normal3d) The front vector of the camera.
     */
    inline Normal3d getFrontVector() const {
        Normal3d front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return Normal3d::fromGlm(glm::normalize(front.toGlm()));
    }

    /**
     * @brief Get the velocity of the camera based on its front vector and
     * movement speed.
     *
     * @return (Magnitude3d) The velocity of the camera.
     */
    inline Magnitude3d getVelocity() const {
        return getFrontVector() * movementSpeed;
    }

  private:
    float lastFrame;

    float yaw = -90.0f;
    float pitch = 0.0f;

    float targetYaw = -90.0f;
    float targetPitch = 0.0f;
};

/**
 * @brief Bundle of camera state values passed to systems that need view
 * context (e.g., weather, atmosphere).
 */
struct ViewInformation {
    /**
     * @brief Current world-space position of the observing camera.
     */
    Position3d position;
    /**
     * @brief World-space point the camera is tracking or looking at.
     */
    Position3d target;
    /**
     * @brief Absolute engine time, typically measured in seconds.
     */
    float time;
    /**
     * @brief Time elapsed since the previous update.
     */
    float deltaTime;
};

#endif // CAMERA_H
