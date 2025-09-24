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
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>

int Contact::compareTo(const Contact other) const {
    if (this->timeOfImpact < other.timeOfImpact) {
        return -1;
    } else if (this->timeOfImpact > other.timeOfImpact) {
        return 1;
    } else {
        return 0;
    }
}

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

    float dt = window.getDeltaTime();
    dt = std::min(dt, 0.0333f);

    // Apply gravity
    float mass = 1.0 / invMass;
    glm::vec3 impulseGravity =
        glm::vec3(0.0f, -window.gravity, 0.0f) * mass * dt;
    applyLinearImpulse(impulseGravity);

    std::vector<std::shared_ptr<Body>> bodies = window.getAllBodies();

    // Check contacts
    int numContacts = 0;
    const int maxContacts = bodies.size() * bodies.size();
    std::vector<Contact> contacts;
    contacts.reserve(maxContacts);
    for (const auto &other : bodies) {
        if (other.get() == this) {
            continue;
        }

        if (invMass == 0.0f && other->invMass == 0.0f) {
            continue;
        }

        Contact contact;
        if (intersects(thisShared, other, contact, dt)) {
            contacts.push_back(contact);
            numContacts++;
            if (numContacts >= maxContacts) {
                break;
            }
        }
    }

    // Sort contacts by time of impact
    if (numContacts > 1) {
        std::sort(contacts.begin(), contacts.end(),
                  [](const Contact &a, const Contact &b) {
                      return a.compareTo(b) < 0;
                  });
    }

    float accumulatedTime = 0.0f;
    for (int i = 0; i < numContacts; i++) {
        Contact &contact = contacts[i];
        const float delta = contact.timeOfImpact - accumulatedTime;

        std::shared_ptr<Body> bodyA = contact.bodyA;
        std::shared_ptr<Body> bodyB = contact.bodyB;

        if (bodyA->invMass == 0.0f && bodyB->invMass == 0.0f) {
            continue;
        }

        for (int j = 0; j < bodies.size(); j++) {
            bodies[j]->updatePhysics(delta);
        }

        resolveContact(contact);
        accumulatedTime += delta;
    }

    // Update the positions for the rest of the frame
    const float remainingTime = dt - accumulatedTime;
    if (remainingTime > 0.0f) {
        for (int i = 0; i < bodies.size(); i++) {
            bodies[i]->updatePhysics(remainingTime);
        }
    }
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
                      Contact &contact, float dt) const {
    if (this->shape == nullptr || other->shape == nullptr) {
        return false;
    }
    contact.bodyA = body;
    contact.bodyB = other;

    if (body->shape->getType() == Shape::ShapeType::Sphere &&
        other->shape->getType() == Shape::ShapeType::Sphere) {
        const Sphere *sphereA = dynamic_cast<const Sphere *>(body->shape.get());
        const Sphere *sphereB =
            dynamic_cast<const Sphere *>(other->shape.get());

        glm::vec3 posA = body->position.toGlm();
        glm::vec3 posB = other->position.toGlm();

        glm::vec3 velA = body->linearVelocity;
        glm::vec3 velB = other->linearVelocity;

        if (bezel::sphereToSphereDynamic(
                sphereA, sphereB, posA, posB, velA, velB, dt,
                contact.pointA.worldSpacePoint, contact.pointB.worldSpacePoint,
                contact.timeOfImpact)) {
            body->updatePhysics(contact.timeOfImpact);
            other->updatePhysics(contact.timeOfImpact);

            contact.pointA.modelSpacePoint =
                body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
            contact.pointB.modelSpacePoint =
                other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

            contact.normal = body->position.toGlm() - other->position.toGlm();
            contact.normal = glm::normalize(contact.normal);

            body->updatePhysics(-contact.timeOfImpact);
            other->updatePhysics(-contact.timeOfImpact);

            glm::vec3 ab =
                contact.pointB.worldSpacePoint - contact.pointA.worldSpacePoint;
            float r = glm::length(ab);
            contact.separationDistance = r;
            return true;
        }
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

    // Get the world space velocity of the contact points
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

    // Calculate the friction
    float frictionA = bodyA->friction;
    float frictionB = bodyB->friction;
    float friction = frictionA * frictionB;

    glm::vec3 velNorm = n * glm::dot(n, vab);
    glm::vec3 velTangent = vab - velNorm;

    glm::vec3 relativeVelTangent(0.0f);
    if (glm::length2(velTangent) > 1e-8f) {
        relativeVelTangent = glm::normalize(velTangent);
    }

    glm::vec3 inertiaA =
        glm::cross((invWorldInertiaA * glm::cross(ra, relativeVelTangent)), ra);
    glm::vec3 inertiaB =
        glm::cross((invWorldInertiaB * glm::cross(rb, relativeVelTangent)), rb);
    const float invInertia = glm::dot(inertiaA + inertiaB, relativeVelTangent);

    float reducedMass = 1.0f / (invMassA + invMassB + invInertia);
    glm::vec3 impulseFriction = velTangent * reducedMass * friction;

    bodyA->applyImpulse(pointA, -impulseFriction);
    bodyB->applyImpulse(pointB, impulseFriction);

    if (contact.timeOfImpact == 0.0f) {
        return; // No positional correction needed
    }

    // Positional correction to avoid sinking

    const Sphere *sphereA = dynamic_cast<const Sphere *>(this->shape.get());
    const Sphere *sphereB = dynamic_cast<const Sphere *>(bodyB->shape.get());

    glm::vec3 centerToCenter = bodyB->position.toGlm() - this->position.toGlm();
    float distance = glm::length(centerToCenter);
    float totalRadius = sphereA->radius + sphereB->radius;

    if (distance < totalRadius) {
        float tA = bodyA->invMass / (bodyA->invMass + bodyB->invMass);
        float tB = bodyB->invMass / (bodyA->invMass + bodyB->invMass);

        const float percent = 0.2f; // correction strength
        const float slop = 0.001f;  // small allowance
        float penetrationDepth = totalRadius - distance;
        float correction = std::max(penetrationDepth - slop, 0.0f) * percent;

        glm::vec3 separationDirection = glm::normalize(centerToCenter);

        bodyA->position =
            bodyA->position -
            Position3d::fromGlm(separationDirection * correction * tA);
        bodyB->position =
            bodyB->position +
            Position3d::fromGlm(separationDirection * correction * tB);
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
    if (invMass == 0.0f) {
        return;
    }
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

    if (angle > 1e-6f) {
        glm::vec3 axis = dAngle / angle; // Safe normalization
        glm::quat dq = glm::angleAxis(angle, axis);
        orientationQuat = glm::normalize(dq * orientationQuat);

        this->orientation = orientationQuat;
        glm::vec3 newPos = positionCM + orientation * (dq * cmToPos);
        this->position = Position3d::fromGlm(newPos);
    }
}
