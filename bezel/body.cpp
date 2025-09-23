/*
 body.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Body calculations
 Copyright (c) 2025 maxvdec
*/

#include "bezel/body.h"
#include "atlas/units.h"
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
    if (thisShared == nullptr) {
        for (const auto &body : window.getAllBodies()) {
            if (body.get() == this) {
                thisShared = body;
                break;
            }
        }
    }

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

        Contact contact;

        if (this->intersects(thisShared, other, contact)) {
            resolveContact(contact);
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

bool Body::intersects(std::shared_ptr<Body> body, std::shared_ptr<Body> other,
                      Contact &contact) const {
    if (this->shape == nullptr || other->shape == nullptr) {
        return false;
    }
    contact.bodyA = body;
    contact.bodyB = other;

    glm::vec3 ab = other->position.toGlm() - this->position.toGlm();
    contact.normal = glm::normalize(ab);

    const Sphere *sphereA = dynamic_cast<const Sphere *>(this->shape.get());
    const Sphere *sphereB = dynamic_cast<const Sphere *>(other->shape.get());

    contact.pointA.worldSpacePoint =
        this->position.toGlm() + contact.normal * sphereA->radius;
    contact.pointB.worldSpacePoint =
        other->position.toGlm() - contact.normal * sphereB->radius;

    float radiusAB = sphereA->radius + sphereB->radius;
    float lengthSquare = glm::dot(ab, ab);
    if (lengthSquare <= (radiusAB * radiusAB)) {
        return true;
    }

    return false;
}

void Body::resolveContact(Contact &contact) {
    std::shared_ptr<Body> bodyA = contact.bodyA;
    std::shared_ptr<Body> bodyB = contact.bodyB;

    const float invMassA = bodyA->invMass;
    const float invMassB = bodyB->invMass;

    const float elasticityA = bodyA->elasticity;
    const float elasticityB = bodyB->elasticity;
    const float elasticity = elasticityA + elasticityB;

    glm::vec3 n = contact.normal;
    glm::vec3 vab = bodyA->linearVelocity - bodyB->linearVelocity;
    float impulseJ =
        -(1.0f + elasticity) * glm::dot(vab, n) / (invMassA + invMassB);
    glm::vec3 impulseJVec = impulseJ * n;

    bodyA->applyLinearImpulse(impulseJVec * 1.0f);
    bodyB->applyLinearImpulse(impulseJVec * -1.0f);

    const Sphere *sphereA = dynamic_cast<const Sphere *>(this->shape.get());
    const Sphere *sphereB = dynamic_cast<const Sphere *>(bodyB->shape.get());

    glm::vec3 centerToCenter = bodyB->position.toGlm() - this->position.toGlm();
    float distance = glm::length(centerToCenter);
    float totalRadius = sphereA->radius + sphereB->radius;

    if (distance < totalRadius) {
        float penetrationDepth = totalRadius - distance;

        float separationDistance = penetrationDepth + 0.001f;

        glm::vec3 separationDirection = glm::normalize(centerToCenter);

        float tA = bodyA->invMass / (bodyA->invMass + bodyB->invMass);
        float tB = bodyB->invMass / (bodyA->invMass + bodyB->invMass);

        bodyA->position =
            bodyA->position -
            Position3d::fromGlm(separationDirection * separationDistance * tA);
        bodyB->position =
            bodyB->position +
            Position3d::fromGlm(separationDirection * separationDistance * tB);
    }
}
