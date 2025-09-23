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
#include "bezel/shape.h"
#include <iostream>
#include <memory>
#include <vector>

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
    std::cout << "Delta time: " << window.getDeltaTime() << " seconds\n";
    std::cout << "Position: " << position << "\n";
    float gravity = window.gravity;
    glm::vec3 gravityVec = {0.0f, -gravity, 0.0f};

    float deltaTime = window.getDeltaTime();
    deltaTime = std::min(deltaTime, 0.0333f);
    float mass = getMass();
    glm::vec3 gravityImpulse = gravityVec * deltaTime * mass;
    applyLinearImpulse(gravityImpulse);

    std::vector<std::shared_ptr<Body>> bodies = window.getAllBodies();
    for (const auto &other : bodies) {
        if (other.get() == this) {
            continue;
        }

        if (other->invMass == 0.0f && this->invMass == 0.0f) {
            continue;
        }

        if (this->intersects(*other)) {
            other->linearVelocity = {0.0f, 0.0f, 0.0f};
            this->linearVelocity = {0.0f, 0.0f, 0.0f};
        }
    }

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

bool Body::intersects(const Body &other) const {
    if (this->shape == nullptr || other.shape == nullptr) {
        return false;
    }

    glm::vec3 ab = other.position.toGlm() - this->position.toGlm();

    const Sphere *sphereA = dynamic_cast<const Sphere *>(this->shape.get());
    const Sphere *sphereB = dynamic_cast<const Sphere *>(other.shape.get());

    float radiusAB = sphereA->radius + sphereB->radius;
    float lengthSquare = glm::dot(ab, ab);
    if (lengthSquare <= (radiusAB * radiusAB)) {
        return true;
    }

    return false;
}
