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

class Camera {
  public:
    Position3d position;
    Point3d target;

    float fov = 45.0f; // Field of view in degrees
    float nearClip = 0.1f;
    float farClip = 100.0f;
    float orthographicSize = 5.0f;

    float movementSpeed = 4.f;
    float mouseSensitivity = 0.1f;
    float lookSmoothness = 0.15f;

    bool useOrthographic = false;

    void move(const Position3d &delta);
    void setPosition(const Position3d &newPosition);
    void lookAt(const Point3d &newTarget);

    Camera() : position({0.0f, 0.0f, 3.0f}), target({0.0f, 0.0f, 0.0f}) {}

    void update(Window &window);
    void updateLook(Window &window, Movement2d movement);
    void updateZoom(Window &window, Movement2d offset);

    void moveTo(Direction3d direction, float speed);

    glm::mat4 calculateViewMatrix() const;

  private:
    float lastFrame;

    float yaw = -90.0f;
    float pitch = 0.0f;

    float targetYaw = -90.0f;
    float targetPitch = 0.0f;
};

#endif // CAMERA_H
