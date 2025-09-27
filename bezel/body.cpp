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
#include "bezel/constraint.h"
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

        if (bodyA != this->thisShared && bodyB != this->thisShared) {
            continue; // Only process contacts involving this body
        }

        if (bodyA->invMass == 0.0f && bodyB->invMass == 0.0f) {
            continue; // Both bodies have infinite mass, skip
        }

        Contact contact;
        if (intersects(bodyA, bodyB, contact, dt)) {
            bool isFreshCollision = contact.timeOfImpact > 1e-6f;
            bool isSignificantPenetration =
                contact.separationDistance < -0.0005f;
            if (isSignificantPenetration || isFreshCollision) {
                contacts[numContacts] = contact;
                numContacts++;
            }
        }
    }

    if (numContacts > 1) {
        std::sort(contacts.begin(), contacts.begin() + numContacts,
                  [](const Contact &a, const Contact &b) {
                      return a.timeOfImpact < b.timeOfImpact;
                  });
    }

    if (!window.solvedConstraints) {
        window.solvedConstraints = true;
        std::vector<Constraint *> constraints = window.getAllConstraints();
        for (auto &constraint : constraints) {
            constraint->preSolve(dt);
        }

        for (auto &constraint : constraints) {
            constraint->solve();
        }

        for (auto &constraint : constraints) {
            constraint->postSolve();
        }
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

    glm::vec3 n = contact.normal;

    bool isSphereBox = (bodyA->shape->getType() == Shape::ShapeType::Sphere &&
                        bodyB->shape->getType() == Shape::ShapeType::Box) ||
                       (bodyB->shape->getType() == Shape::ShapeType::Sphere &&
                        bodyA->shape->getType() == Shape::ShapeType::Box);

    if (isSphereBox) {
        float absX = std::abs(n.x);
        float absY = std::abs(n.y);
        float absZ = std::abs(n.z);

        const float precision_threshold = 0.001f;

        if (absY > absX && absY > absZ) {
            if (absX < precision_threshold || absZ < precision_threshold) {
                n = glm::vec3(0.0f, n.y > 0 ? 1.0f : -1.0f, 0.0f);
            }
        } else if (absX > absY && absX > absZ) {
            if (absY < precision_threshold || absZ < precision_threshold) {
                n = glm::vec3(n.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
            }
        } else {
            if (absX < precision_threshold || absY < precision_threshold) {
                n = glm::vec3(0.0f, 0.0f, n.z > 0 ? 1.0f : -1.0f);
            }
        }
    }

    contact.normal = n;

    glm::vec3 pointA = contact.pointA.worldSpacePoint;
    glm::vec3 pointB = contact.pointB.worldSpacePoint;

    if (!std::isfinite(pointA.x) || !std::isfinite(pointA.y) ||
        !std::isfinite(pointA.z) || !std::isfinite(pointB.x) ||
        !std::isfinite(pointB.y) || !std::isfinite(pointB.z)) {
        return;
    }

    const float invMassA = bodyA->invMass;
    const float invMassB = bodyB->invMass;

    const float elasticityA = bodyA->elasticity;
    const float elasticityB = bodyB->elasticity;
    const float elasticity = elasticityA * elasticityB;

    glm::mat3 invWorldInertiaA = bodyA->getInverseInertiaTensorWorldSpace();
    glm::mat3 invWorldInertiaB = bodyB->getInverseInertiaTensorWorldSpace();

    if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
        return;
    }

    if (glm::length2(n) < 1e-8f) {
        return;
    }

    n = glm::normalize(n);

    glm::vec3 ra = pointA - bodyA->getCenterOfMassWorldSpace();
    glm::vec3 rb = pointB - bodyB->getCenterOfMassWorldSpace();

    glm::vec3 velA = bodyA->linearVelocity;
    glm::vec3 velB = bodyB->linearVelocity;

    if (invMassA > 0.0f) {
        velA += glm::cross(bodyA->angularVelocity, ra);
    }
    if (invMassB > 0.0f) {
        velB += glm::cross(bodyB->angularVelocity, rb);
    }

    glm::vec3 relativeVel = velA - velB;
    float normalVel = glm::dot(relativeVel, n);

    bool isPenetrating = contact.separationDistance < -0.001f;

    if (isPenetrating && contact.separationDistance < -0.01f) {
        float totalInvMass = invMassA + invMassB;
        if (totalInvMass > 1e-8f) {
            const float percent = 0.05f;
            const float slop = 0.01f;
            float penetrationDepth = -contact.separationDistance;

            if (penetrationDepth > slop) {
                float correction =
                    (penetrationDepth - slop) * percent / totalInvMass;
                glm::vec3 correctionVec = n * correction;

                if (std::isfinite(correctionVec.x) &&
                    std::isfinite(correctionVec.y) &&
                    std::isfinite(correctionVec.z)) {
                    if (invMassA > 0.0f) {
                        bodyA->position =
                            bodyA->position +
                            Position3d::fromGlm(correctionVec * invMassA);
                    }
                    if (invMassB > 0.0f) {
                        bodyB->position =
                            bodyB->position -
                            Position3d::fromGlm(correctionVec * invMassB);
                    }
                }
            }
        }
    }

    bool isResting =
        std::abs(normalVel) < 1.0f && contact.separationDistance > -0.05f;

    if (isResting) {

        if (normalVel < -0.01f) {
            glm::vec3 angularJA = glm::vec3(0.0f);
            glm::vec3 angularJB = glm::vec3(0.0f);

            if (invMassA > 0.0f) {
                angularJA =
                    glm::cross((invWorldInertiaA * glm::cross(ra, n)), ra);
            }
            if (invMassB > 0.0f) {
                angularJB =
                    glm::cross((invWorldInertiaB * glm::cross(rb, n)), rb);
            }

            const float angularFactor = glm::dot(angularJA + angularJB, n);
            const float denominator = invMassA + invMassB + angularFactor;

            if (std::abs(denominator) > 1e-8f) {
                float stabilizingImpulse = -normalVel / denominator;
                glm::vec3 normalImpulse = stabilizingImpulse * n;

                if (std::isfinite(normalImpulse.x) &&
                    std::isfinite(normalImpulse.y) &&
                    std::isfinite(normalImpulse.z)) {
                    bodyA->applyImpulse(pointA, normalImpulse);
                    bodyB->applyImpulse(pointB, -normalImpulse);
                }
            }
        }

        glm::vec3 tangentialVel = relativeVel - glm::dot(relativeVel, n) * n;
        float tangentialSpeed = glm::length(tangentialVel);

        if (tangentialSpeed > 0.01f) {
            glm::vec3 frictionDir = tangentialVel / tangentialSpeed;

            float frictionA = bodyA->friction;
            float frictionB = bodyB->friction;
            float combinedFriction = std::sqrt(frictionA * frictionB);

            glm::vec3 frictionAngularJA = glm::vec3(0.0f);
            glm::vec3 frictionAngularJB = glm::vec3(0.0f);

            if (invMassA > 0.0f) {
                frictionAngularJA = glm::cross(
                    (invWorldInertiaA * glm::cross(ra, frictionDir)), ra);
            }
            if (invMassB > 0.0f) {
                frictionAngularJB = glm::cross(
                    (invWorldInertiaB * glm::cross(rb, frictionDir)), rb);
            }

            float frictionAngularFactor =
                glm::dot(frictionAngularJA + frictionAngularJB, frictionDir);
            float frictionDenominator =
                invMassA + invMassB + frictionAngularFactor;

            if (std::abs(frictionDenominator) > 1e-8f) {
                if (combinedFriction >= 0.5f) {
                    float frictionImpulseJ =
                        -tangentialSpeed / frictionDenominator;
                    glm::vec3 frictionImpulse = frictionImpulseJ * frictionDir;

                    if (std::isfinite(frictionImpulse.x) &&
                        std::isfinite(frictionImpulse.y) &&
                        std::isfinite(frictionImpulse.z)) {
                        bodyA->applyImpulse(pointA, frictionImpulse);
                        bodyB->applyImpulse(pointB, -frictionImpulse);
                    }
                } else {
                    float maxFriction = combinedFriction * 100.0f;
                    float desiredFrictionImpulse =
                        -tangentialSpeed / frictionDenominator;
                    float frictionImpulseJ =
                        std::max(-maxFriction,
                                 std::min(maxFriction, desiredFrictionImpulse));

                    glm::vec3 frictionImpulse = frictionImpulseJ * frictionDir;

                    if (std::isfinite(frictionImpulse.x) &&
                        std::isfinite(frictionImpulse.y) &&
                        std::isfinite(frictionImpulse.z)) {
                        bodyA->applyImpulse(pointA, frictionImpulse);
                        bodyB->applyImpulse(pointB, -frictionImpulse);
                    }
                }
            }
        }

        if (tangentialSpeed < 0.1f) {
            const float dampingFactor = 0.95f;
            if (invMassA > 0.0f) {
                bodyA->linearVelocity *= dampingFactor;
                bodyA->angularVelocity *= dampingFactor;
            }
            if (invMassB > 0.0f) {
                bodyB->linearVelocity *= dampingFactor;
                bodyB->angularVelocity *= dampingFactor;
            }
        }

        return;
    }

    if (normalVel >= -0.01f) {
        return;
    }

    glm::vec3 angularJA = glm::vec3(0.0f);
    glm::vec3 angularJB = glm::vec3(0.0f);

    if (invMassA > 0.0f) {
        angularJA = glm::cross((invWorldInertiaA * glm::cross(ra, n)), ra);
    }
    if (invMassB > 0.0f) {
        angularJB = glm::cross((invWorldInertiaB * glm::cross(rb, n)), rb);
    }

    const float angularFactor = glm::dot(angularJA + angularJB, n);
    const float denominator = invMassA + invMassB + angularFactor;

    if (std::abs(denominator) < 1e-8f) {
        return;
    }

    float impulseJ = -(1.0f + elasticity) * normalVel / denominator;

    const float maxImpulse = 1000.0f;
    if (std::abs(impulseJ) > maxImpulse) {
        impulseJ = (impulseJ > 0.0f) ? maxImpulse : -maxImpulse;
    }

    glm::vec3 normalImpulse = impulseJ * n;

    if (!std::isfinite(normalImpulse.x) || !std::isfinite(normalImpulse.y) ||
        !std::isfinite(normalImpulse.z)) {
        return;
    }

    bodyA->applyImpulse(pointA, normalImpulse);
    bodyB->applyImpulse(pointB, -normalImpulse);

    glm::vec3 newVelA = bodyA->linearVelocity;
    glm::vec3 newVelB = bodyB->linearVelocity;

    if (invMassA > 0.0f) {
        newVelA += glm::cross(bodyA->angularVelocity, ra);
    }
    if (invMassB > 0.0f) {
        newVelB += glm::cross(bodyB->angularVelocity, rb);
    }

    glm::vec3 newRelativeVel = newVelA - newVelB;
    glm::vec3 tangentialVel = newRelativeVel - glm::dot(newRelativeVel, n) * n;
    float tangentialSpeed = glm::length(tangentialVel);

    if (tangentialSpeed > 1e-6f) {
        glm::vec3 frictionDir = tangentialVel / tangentialSpeed;

        float frictionA = bodyA->friction;
        float frictionB = bodyB->friction;
        float combinedFriction = std::sqrt(frictionA * frictionB);

        glm::vec3 frictionAngularJA = glm::vec3(0.0f);
        glm::vec3 frictionAngularJB = glm::vec3(0.0f);

        if (invMassA > 0.0f) {
            frictionAngularJA = glm::cross(
                (invWorldInertiaA * glm::cross(ra, frictionDir)), ra);
        }
        if (invMassB > 0.0f) {
            frictionAngularJB = glm::cross(
                (invWorldInertiaB * glm::cross(rb, frictionDir)), rb);
        }

        float frictionAngularFactor =
            glm::dot(frictionAngularJA + frictionAngularJB, frictionDir);
        float frictionDenominator = invMassA + invMassB + frictionAngularFactor;

        if (std::abs(frictionDenominator) > 1e-8f) {
            float frictionImpulseJ = -tangentialSpeed / frictionDenominator;

            float maxFrictionImpulse = combinedFriction * std::abs(impulseJ);

            if (std::abs(frictionImpulseJ) > maxFrictionImpulse) {
                frictionImpulseJ = (frictionImpulseJ > 0.0f)
                                       ? maxFrictionImpulse
                                       : -maxFrictionImpulse;
            }

            glm::vec3 frictionImpulse = frictionImpulseJ * frictionDir;
            bodyA->applyImpulse(pointA, frictionImpulse);
            bodyB->applyImpulse(pointB, -frictionImpulse);
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