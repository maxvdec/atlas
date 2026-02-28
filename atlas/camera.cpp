/*
 camera.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Camera function implementations
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/input.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

glm::mat4 Camera::calculateViewMatrix() const {
    glm::dvec3 camPos(position.x, position.y, position.z);
    glm::dvec3 camTarget(target.x, target.y, target.z);
    glm::dvec3 upVector(0.0, 1.0, 0.0); // Assuming Y-up coordinate system

    return glm::mat4(glm::lookAt(camPos, camTarget, upVector));
}

void Camera::move(const Position3d &delta) {
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
}

void Camera::setPosition(const Position3d &newPosition) {
    position = newPosition;
}

void Camera::lookAt(const Point3d &newTarget) {
    target = newTarget;
    glm::vec3 dir = glm::normalize(glm::vec3(newTarget.x - position.x,
                                             newTarget.y - position.y,
                                             newTarget.z - position.z));
    pitch = glm::degrees(asin(dir.y));
    yaw = glm::degrees(atan2(dir.z, dir.x));
    targetPitch = pitch;
    targetYaw = yaw;
}

void Camera::setPositionKeepingOrientation(const Position3d &newPos) {
    glm::dvec3 oldPos(position.x, position.y, position.z);
    glm::dvec3 oldTgt(target.x, target.y, target.z);
    glm::dvec3 forward = glm::normalize(oldTgt - oldPos);

    position = newPos;
    target = {position.x + forward.x, position.y + forward.y,
              position.z + forward.z};
}

void Camera::moveTo(Direction3d direction, float speed) {
    glm::vec3 camPos = glm::vec3(position.x, position.y, position.z);
    glm::vec3 camFront =
        glm::normalize(glm::vec3(target.x, target.y, target.z) - camPos);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f); // Assuming Y-up coordinate system

    switch (direction) {
    case Direction3d::Forward:
        camPos += speed * camFront;
        break;
    case Direction3d::Backward:
        camPos -= speed * camFront;
        break;
    case Direction3d::Left:
        camPos -= glm::normalize(glm::cross(camFront, upVector)) * speed;
        break;
    case Direction3d::Right:
        camPos += glm::normalize(glm::cross(camFront, upVector)) * speed;
        break;
    case Direction3d::Up:
        camPos += upVector * speed;
        break;
    case Direction3d::Down:
        camPos -= upVector * speed;
        break;
    }

    position = {camPos.x, camPos.y, camPos.z};
    target = {camPos.x + camFront.x, camPos.y + camFront.y,
              camPos.z + camFront.z};
}

void Camera::update(Window &window) {
    glm::vec3 camPos = glm::vec3(position.x, position.y, position.z);
    glm::vec3 camFront =
        glm::normalize(glm::vec3(target.x, target.y, target.z) - camPos);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f); // Assuming Y-up coordinate system

    float deltaTime = window.getDeltaTime();

    float cameraSpeed = movementSpeed * deltaTime;

    if (window.isKeyPressed(Key::W) || window.isKeyPressed(Key::Up)) {
        camPos += cameraSpeed * camFront;
    }
    if (window.isKeyPressed(Key::S) || window.isKeyPressed(Key::Down)) {
        camPos -= cameraSpeed * camFront;
    }
    if (window.isKeyPressed(Key::A) || window.isKeyPressed(Key::Left)) {
        camPos -= glm::normalize(glm::cross(camFront, upVector)) * cameraSpeed;
    }
    if (window.isKeyPressed(Key::D) || window.isKeyPressed(Key::Right)) {
        camPos += glm::normalize(glm::cross(camFront, upVector)) * cameraSpeed;
    }
    if (window.isKeyPressed(Key::Space)) {
        camPos.y += cameraSpeed;
    }
    if (window.isKeyPressed(Key::LeftShift)) {
        camPos.y -= cameraSpeed;
    }

    position = {camPos.x, camPos.y, camPos.z};
    target = {camPos.x + camFront.x, camPos.y + camFront.y,
              camPos.z + camFront.z};
}

void Camera::updateLook(Window &, Movement2d movement) {
    float xoffset = movement.x * mouseSensitivity;
    float yoffset = movement.y * mouseSensitivity;

    targetYaw += xoffset;
    targetPitch += yoffset;

    if (targetPitch > 89.0f)
        targetPitch = 89.0f;
    if (targetPitch < -89.0f)
        targetPitch = -89.0f;

    yaw += (targetYaw - yaw) * lookSmoothness;
    pitch += (targetPitch - pitch) * lookSmoothness;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    target = {position.x + front.x, position.y + front.y, position.z + front.z};
}

void Camera::updateZoom(Window &, Movement2d offset) {
    if (!useOrthographic) {
        fov -= offset.y;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 90.0f)
            fov = 90.0f;
    } else {
        orthographicSize -= offset.y * 0.1f;
        if (orthographicSize < 1.0f)
            orthographicSize = 1.0f;
        if (orthographicSize > 20.0f)
            orthographicSize = 20.0f;
    }
}
