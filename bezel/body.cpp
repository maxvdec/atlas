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

    updatePhysics(deltaTime);
}

void Body::applyLinearImpulse(const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }
    linearVelocity += impulse * invMass;
}

void Body::applyAngularImpulse(const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }
    angularVelocity += getInverseInertiaTensorWorldSpace() * impulse;

    const float maxAngularSpeed = 30.0f;
    if (glm::length(angularVelocity) > maxAngularSpeed) {
        angularVelocity = glm::normalize(angularVelocity) * maxAngularSpeed;
    }
}

void Body::applyImpulse(const glm::vec3 &point, const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }

    applyLinearImpulse(impulse);

    glm::vec3 position = getCenterOfMassWorldSpace();
    glm::vec3 r = point - position;
    glm::vec3 angularImpulse = glm::cross(r, impulse);
    applyAngularImpulse(angularImpulse);
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

    glm::vec3 pointA = contact.pointA.worldSpacePoint;
    glm::vec3 pointB = contact.pointB.worldSpacePoint;

    const float invMassA = bodyA->invMass;
    const float invMassB = bodyB->invMass;

    const float elasticityA = bodyA->elasticity;
    const float elasticityB = bodyB->elasticity;
    const float elasticity = elasticityA + elasticityB;

    glm::mat3 invWorldInertiaA = bodyA->getInverseInertiaTensorWorldSpace();
    glm::mat3 invWorldInertiaB = bodyB->getInverseInertiaTensorWorldSpace();

    glm::vec3 n = contact.normal;

    glm::vec3 ra = pointA - bodyA->getCenterOfMassWorldSpace();
    glm::vec3 rb = pointB - bodyB->getCenterOfMassWorldSpace();

    glm::vec3 angularJA =
        glm::cross((invWorldInertiaA * glm::cross(ra, n)), ra);
    glm::vec3 angularJB =
        glm::cross((invWorldInertiaB * glm::cross(rb, n)), rb);
    const float angularFactor = glm::dot(angularJA + angularJB, n);

    // Get the worl space velocity of the contact points
    glm::vec3 velA =
        bodyA->linearVelocity + glm::cross(bodyA->angularVelocity, ra);
    glm::vec3 velB =
        bodyB->linearVelocity + glm::cross(bodyB->angularVelocity, rb);

    glm::vec3 vab = velA - velB;

    float impulseJ = (1.0f + elasticity) * glm::dot(vab, n) /
                     (invMassA + invMassB + angularFactor);
    glm::vec3 impulseJVec = impulseJ * n;

    bodyA->applyImpulse(pointA, -impulseJVec);
    bodyB->applyImpulse(pointB, impulseJVec);

    // Positional correction to avoid sinking

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

glm::mat3 Body::getInverseInertiaTensorBodySpace() const {
    glm::mat3 inertiaTensor = shape->getInertiaTensor();
    glm::mat3 invInertiaTensor = glm::inverse(inertiaTensor) * invMass;
    return invInertiaTensor;
}

glm::mat3 Body::getInverseInertiaTensorWorldSpace() const {
    glm::mat3 inertiaTensor = shape->getInertiaTensor();
    glm::mat3 invInertiaTensor = glm::inverse(inertiaTensor) * invMass;
    glm::mat3 rotationMatrix = glm::mat3_cast(orientation);
    return rotationMatrix * invInertiaTensor * glm::transpose(rotationMatrix);
}

void Body::updatePhysics(double dt) {
    glm::vec3 pos = this->position.toGlm() + linearVelocity * (float)dt;
    this->position = Position3d::fromGlm(pos);

    glm::vec3 positionCM = getCenterOfMassWorldSpace();
    glm::vec3 cmToPos = this->position.toGlm() - positionCM;

    glm::mat3 orientation = glm::mat3_cast(this->orientation);
    glm::mat3 inertiaTensor = orientation * this->shape->getInertiaTensor() *
                              glm::transpose(orientation);
    glm::vec3 alpha =
        glm::inverse(inertiaTensor) *
        glm::cross(angularVelocity, inertiaTensor * angularVelocity);
    angularVelocity += alpha * (float)dt;

    glm::quat orientationQuat = glm::quat_cast(orientation);

    glm::vec3 dAngle = angularVelocity * (float)dt;
    float angle = glm::length(dAngle);
    glm::vec3 axis = glm::normalize(dAngle);
    glm::quat dq = glm::angleAxis(angle, axis);
    orientationQuat = glm::normalize(dq * orientationQuat);

    orientation = glm::mat3_cast(orientationQuat);
    glm::vec3 newPos = positionCM + orientation * (dq * cmToPos);
}
