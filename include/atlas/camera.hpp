/*
 camera.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The Camera setup for the Atlas Engine
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_CAMERA_HPP
#define ATLAS_CAMERA_HPP

#include "atlas/input.hpp"
#include "atlas/units.hpp"
#include <glm/glm.hpp>

#define DEFAULT_YAW -90.0f
#define DEFAULT_PITCH 0.0f
#define DEFAULT_SPPED 2.5f
#define DEFAULT_SENSITIVITY 0.1f
#define DEFAULT_ZOOM 45.0f

class Camera : Interactive {
  public:
    Position3d position;
    Position3d target;
    Position3d up;

    float movementSpeed = DEFAULT_SPPED;
    float mouseSensitivity;
    float zoom;

    Camera(Position3d position = Position3d(0.0f, 0.0f, 0.0f),
           Position3d target = Position3d(0.0f, 0.0f, 0.0f),
           Position3d up = Position3d(0.0f, 1.0f, 0.0f));

    Camera(float posX, float posY, float posZ, float targetX, float targetY,
           float targetZ, float upX, float upY, float upZ);

    void useCamera();
    void atEachFrame(float deltaTime) override;
    void onMouseMove(MousePacket data, float deltaTime) override;
    void onMouseScroll(MouseScrollPacket data, float deltaTime) override;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

  private:
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw = 0;
    float pitch = 0;

    void updateCameraVectors();
};

#endif // ATLAS_CAMERA_HPP
