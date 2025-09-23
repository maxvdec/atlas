/*
 body.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Body definitions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_BODY_H
#define ATLAS_BODY_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include "atlas/units.h"
#include "bezel/shape.h"

class Window;

struct IntersectionPoint {
    glm::vec3 worldSpacePoint;
    glm::vec3 modelSpacePoint;
};

class Body;

struct Contact {
    IntersectionPoint pointA;
    IntersectionPoint pointB;

    glm::vec3 normal;
    float separationDistance;
    float timeOfImpact;

    std::shared_ptr<Body> bodyA;
    std::shared_ptr<Body> bodyB;
};

class Body {
  public:
    Position3d position;
    glm::quat orientation;
    std::shared_ptr<Shape> shape;
    glm::vec3 linearVelocity = {0.0f, 0.0f, 0.0f};
    glm::vec3 angularVelocity = {0.0f, 0.0f, 0.0f};
    float invMass = 0.0f;
    float elasticity = 0.5f;

    glm::vec3 getCenterOfMassWorldSpace() const;
    glm::vec3 getCenterOfMassModelSpace() const;

    glm::vec3 worldSpaceToModelSpace(const glm::vec3 &point) const;
    glm::vec3 modelSpaceToWorldSpace(const glm::vec3 &point) const;

    glm::mat3 getInverseInertiaTensorWorldSpace() const;
    glm::mat3 getInverseInertiaTensorBodySpace() const;

    void applyImpulse(const glm::vec3 &point, const glm::vec3 &impulse);
    void applyLinearImpulse(const glm::vec3 &impulse);
    void applyAngularImpulse(const glm::vec3 &impulse);

    inline void applyMass(float mass) {
        if (mass <= 0.0f) {
            invMass = 0.0f; // Infinite mass
        } else {
            invMass = 1.0f / mass;
        }
    }

    inline float getMass() const {
        if (invMass == 0.0f) {
            return INFINITY;
        }
        return 1.0f / invMass;
    }

    void update(Window &window);

    bool intersects(std::shared_ptr<Body> bodyA, std::shared_ptr<Body> bodyB,
                    Contact &contact) const;

    void resolveContact(Contact &contact);

  private:
    std::shared_ptr<Body> thisShared = nullptr;

    void updatePhysics(double dt);
};

#endif // ATLAS_BODY_H
