/*
 body.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Body calculations
 Copyright (c) 2025 maxvdec
*/

#include "bezel/body.h"
#include "atlas/window.h"
#include <iostream>

glm::vec3 Body::getCenterOfMassWorldSpace() const {
    return modelSpaceToWorldSpace(this->shape->getCenterOfMass());
}

glm::vec3 Body::getCenterOfMassModelSpace() const {
    return this->shape->getCenterOfMass();
}

glm::vec3 Body::worldSpaceToModelSpace(const glm::vec3 &point) const {
    glm::vec3 translated = point - position.toGlm();
    glm::quat invOrientation = glm::conjugate(orientation);
    return invOrientation * translated;
}

glm::vec3 Body::modelSpaceToWorldSpace(const glm::vec3 &point) const {
    glm::vec3 rotated = orientation * point;
    return rotated + position.toGlm();
}

void Body::update(Window &window) {
    float gravity = window.gravity;
    glm::vec3 gravityVec = {0.0f, -gravity, 0.0f};

    float deltaTime = window.getDeltaTime();
    float mass = getMass();
    glm::vec3 gravityImpulse = gravityVec * deltaTime * mass;
    applyLinearImpulse(gravityImpulse);

    // Apply all linear velocity
    glm::vec3 positionGlm = position.toGlm();
    positionGlm += linearVelocity * deltaTime;
    position = Position3d::fromGlm(positionGlm);
}

void Body::applyLinearImpulse(const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }
    linearVelocity += impulse * invMass;
}
