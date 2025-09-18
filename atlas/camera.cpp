/*
 camera.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Camera function implementations
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

glm::mat4 Camera::calculateViewMatrix() const {
    glm::vec3 camPos(position.x, position.y, position.z);
    glm::vec3 camTarget(target.x, target.y, target.z);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f); // Assuming Y-up coordinate system

    return glm::lookAt(camPos, camTarget, upVector);
}

void Camera::move(const Position3d &delta) {
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
}

void Camera::setPosition(const Position3d &newPosition) {
    position = newPosition;
}

void Camera::lookAt(const Point3d &newTarget) { target = newTarget; }
