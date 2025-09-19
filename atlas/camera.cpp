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
#include <iostream>

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

    float currentFrame = window.getTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

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

void Camera::updateLook(Window &window, Movement2d movement) {
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
