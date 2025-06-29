/*
 camera.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Camera implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.hpp"
#include "atlas/input.hpp"
#include "atlas/units.hpp"
#include "atlas/window.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Camera::Camera(Position3d position, Position3d target, Position3d up)
    : position(position.withInverted(Axis::Z)), target(target), up(up),
      movementSpeed(DEFAULT_SPPED), mouseSensitivity(DEFAULT_SENSITIVITY),
      zoom(DEFAULT_ZOOM), worldUp(up.toVec3()),
      front(glm::vec3(0.0f, 0.0f, -1.0f)) {
    Window::current_window->registerInteractive(this);
}

void Camera::atEachFrame(float deltaTime) {
    float velocity = this->movementSpeed * deltaTime;
    if (isKeyPressed(Key::W)) {
        this->position += front * velocity;
    } else if (isKeyPressed(Key::S)) {
        this->position -= front * velocity;
    } else if (isKeyPressed(Key::A)) {
        this->position -=
            glm::normalize(glm::cross(front, up.toVec3())) * velocity;
    } else if (isKeyPressed(Key::D)) {
        this->position +=
            glm::normalize(glm::cross(front, up.toVec3())) * velocity;
    }
    updateCameraVectors();
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(this->zoom), aspectRatio, 0.1f,
                            100.0f);
}

void Camera::onMouseMove(MousePacket data, float deltaTime) {
    data.xoffset *= this->mouseSensitivity;
    data.yoffset *= this->mouseSensitivity;

    this->yaw += data.xoffset;
    this->pitch += data.yoffset;

    if (data.constrainPitch) {
        if (this->pitch > 89.0f) {
            this->pitch = 89.0f;
        }
        if (this->pitch < -89.0f) {
            this->pitch = -89.0f;
        }
    }
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position.toVec3(), position.toVec3() + front,
                       up.toVec3());
}

void Camera::useCamera() { Window::current_window->setMainCamera(this); }

void Camera::onMouseScroll(MouseScrollPacket data, float deltaTime) {
    this->zoom -= data.yoffset;
    if (this->zoom < 1.0f) {
        this->zoom = 1.0f;
    }
    if (this->zoom > 45.0f) {
        this->zoom = 45.0f;
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    front.y = sin(glm::radians(this->pitch));
    front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    this->front = glm::normalize(front);

    this->right = glm::normalize(glm::cross(this->front, this->worldUp));
    this->worldUp = glm::normalize(glm::cross(this->right, this->front));
}
