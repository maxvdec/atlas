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
#include "bezel/bounds.h"
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

    std::cout << "Position: " << glm::to_string(position.toGlm())
              << " Velocity: " << glm::to_string(linearVelocity)
              << " Angular Velocity: " << glm::to_string(angularVelocity)
              << std::endl;

    float dt = window.getDeltaTime();
    dt = std::min(dt, 0.0333f);

    // Apply gravity
    float mass = 1.0 / invMass;
    glm::vec3 impulseGravity =
        glm::vec3(0.0f, -window.gravity, 0.0f) * mass * dt;
    applyLinearImpulse(impulseGravity);

    std::vector<std::shared_ptr<Body>> bodies = window.getAllBodies();

    // Broadphase collision detection using Sweep and Prune
    std::vector<CollisionPair> pairs;
    bezel::broadPhase(bodies, pairs, dt);

    // Narrowphase

    int numContacts = 0;
    const int maxContacts = bodies.size() * bodies.size();
    std::vector<Contact> contacts(maxContacts);
    for (int i = 0; i < pairs.size(); i++) {
        CollisionPair pair = pairs[i];
        std::shared_ptr<Body> bodyA = bodies[pair.a];
        std::shared_ptr<Body> bodyB = bodies[pair.b];

        if (bodyA->invMass == 0.0f && bodyB->invMass == 0.0f) {
            continue; // Both bodies have infinite mass, skip
        }

        Contact contact;
        if (intersects(bodyA, bodyB, contact, dt)) {
            contacts[numContacts] = contact;
            numContacts++;
        }
    }

    if (numContacts > 1) {
        std::sort(contacts.begin(), contacts.begin() + numContacts,
                  [](const Contact &a, const Contact &b) {
                      return a.timeOfImpact < b.timeOfImpact;
                  });
    }

    // Integrate and resolve contacts

    float accumulatedTime = 0.0f;
    for (int i = 0; i < numContacts; i++) {
        Contact &contact = contacts[i];
        float dtContact = contact.timeOfImpact - accumulatedTime;

        updatePhysics(dtContact);
        resolveContact(contact);
        accumulatedTime += dtContact;
    }

    const float remainingTime = dt - accumulatedTime;
    if (remainingTime > 0.0f) {
        updatePhysics(remainingTime);
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

    glm::vec3 velA =
        bodyA->linearVelocity + glm::cross(bodyA->angularVelocity, ra);
    glm::vec3 velB =
        bodyB->linearVelocity + glm::cross(bodyB->angularVelocity, rb);

    glm::vec3 vab = velA - velB;

    const float denominator = invMassA + invMassB + angularFactor;
    if (std::abs(denominator) < 1e-6f) {
        return; // Skip resolution if denominator is too small
    }

    float impulseJ = (1.0f + elasticity) * glm::dot(vab, n) / denominator;
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

    if (invMassA == 0.0f && invMassB == 0.0f) {
        return;
    }

    const Sphere *sphereA = dynamic_cast<const Sphere *>(bodyA->shape.get());
    const Sphere *sphereB = dynamic_cast<const Sphere *>(bodyB->shape.get());

    if (sphereA && sphereB) {
        glm::vec3 centerToCenter =
            bodyB->position.toGlm() - bodyA->position.toGlm();
        float distance = glm::length(centerToCenter);
        float totalRadius = sphereA->radius + sphereB->radius;

        if (distance < totalRadius) {
            float totalInvMass = invMassA + invMassB;

            if (totalInvMass > 1e-8f) {
                float tA = invMassA / totalInvMass;
                float tB = invMassB / totalInvMass;

                const float percent = 0.2f; // correction strength
                const float slop = 0.001f;  // small allowance
                float penetrationDepth = totalRadius - distance;
                float correction =
                    std::max(penetrationDepth - slop, 0.0f) * percent;

                glm::vec3 separationDirection = glm::normalize(centerToCenter);

                if (invMassA > 0.0f) {
                    bodyA->position = bodyA->position -
                                      Position3d::fromGlm(separationDirection *
                                                          correction * tA);
                }
                if (invMassB > 0.0f) {
                    bodyB->position = bodyB->position +
                                      Position3d::fromGlm(separationDirection *
                                                          correction * tB);
                }
            }
        }
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

    if (glm::length2(angularVelocity) > 1e-12f) {
        glm::mat3 orientation = glm::mat3_cast(this->orientation);

        glm::mat3 inertiaTensor = orientation *
                                  this->shape->getInertiaTensor() *
                                  glm::transpose(orientation);

        float det = glm::determinant(inertiaTensor);
        if (std::abs(det) > 1e-12f) {
            glm::vec3 L = inertiaTensor * angularVelocity;
            glm::vec3 torque = glm::cross(angularVelocity, L);
            glm::vec3 alpha = glm::inverse(inertiaTensor) * torque;

            angularVelocity += alpha * (float)dt;

            const float maxAngularSpeed = 30.0f;
            if (glm::length2(angularVelocity) >
                maxAngularSpeed * maxAngularSpeed) {
                angularVelocity =
                    glm::normalize(angularVelocity) * maxAngularSpeed;
            }
        }

        glm::vec3 dAngle = angularVelocity * (float)dt;
        float angle = glm::length(dAngle);

        if (angle > 1e-8f) {
            glm::vec3 axis = dAngle / angle;
            glm::quat dq = glm::angleAxis(angle, axis);

            if (glm::length2(glm::vec4(dq.x, dq.y, dq.z, dq.w)) > 1e-12f &&
                glm::length2(glm::vec4(this->orientation.x, this->orientation.y,
                                       this->orientation.z,
                                       this->orientation.w)) > 1e-12f) {

                this->orientation = glm::normalize(dq * this->orientation);

                if (!std::isfinite(this->orientation.x) ||
                    !std::isfinite(this->orientation.y) ||
                    !std::isfinite(this->orientation.z) ||
                    !std::isfinite(this->orientation.w)) {
                    this->orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    angularVelocity = glm::vec3(0.0f);
                }
            }
        }
    }

    glm::vec3 finalPos = this->position.toGlm();
    if (!std::isfinite(finalPos.x) || !std::isfinite(finalPos.y) ||
        !std::isfinite(finalPos.z)) {
        std::cerr << "ERROR: Invalid position detected! Resetting to origin."
                  << std::endl;
        this->position = Position3d::fromGlm(glm::vec3(0.0f, 5.0f, 0.0f));
        this->linearVelocity = glm::vec3(0.0f);
        this->angularVelocity = glm::vec3(0.0f);
    }
}

Point bezel::support(const std::shared_ptr<Body> bodyA,
                     const std::shared_ptr<Body> bodyB, glm::vec3 dir,
                     float bias) {
    dir = glm::normalize(dir);

    Point point;

    point.ptA = bodyA->shape->support(dir, bodyA->position.toGlm(),
                                      bodyA->orientation, bias);

    dir *= -1.0f;

    point.ptB = bodyB->shape->support(dir, bodyB->position.toGlm(),
                                      bodyB->orientation, bias);

    point.xyz = point.ptA - point.ptB;
    return point;
}