//
// collisions.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Collision detection functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/body.h"
#include "bezel/shape.h"
#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>

bool bezel::collisions::sphereToSphereStatic(
    const Sphere *sphereA, const Sphere *sphereB, const glm::vec3 &posA,
    const glm::vec3 &posB, glm::vec3 &pointOnA, glm::vec3 &pointOnB) {

    const glm::vec3 ab = posB - posA;
    glm::vec3 norm = glm::normalize(ab);

    pointOnA = posA + norm * sphereA->radius;
    pointOnB = posB - norm * sphereB->radius;

    const float radiusAB = sphereA->radius + sphereB->radius;
    const float lengthSquare = glm::length2(ab);
    if (lengthSquare <= (radiusAB * radiusAB)) {
        return true;
    }

    return false;
}

bool Body::intersects(std::shared_ptr<Body> body, std::shared_ptr<Body> other,
                      Contact &contact, float dt) const {
    contact.bodyA = body;
    contact.bodyB = other;
    contact.timeOfImpact = 0.0f;

    if (body->shape->getType() == Shape::ShapeType::Sphere &&
        other->shape->getType() == Shape::ShapeType::Sphere) {
        auto sphereA = dynamic_cast<Sphere *>(body->shape.get());
        auto sphereB = dynamic_cast<Sphere *>(other->shape.get());

        glm::vec3 posA = body->position.toGlm();
        glm::vec3 posB = other->position.toGlm();

        if (bezel::collisions::sphereToSphereStatic(
                sphereA, sphereB, posA, posB, contact.pointA.worldSpacePoint,
                contact.pointB.worldSpacePoint)) {
            contact.normal = glm::normalize(posB - posA);

            contact.pointA.modelSpacePoint =
                body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
            contact.pointB.modelSpacePoint =
                other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

            glm::vec3 ab =
                contact.pointB.worldSpacePoint - contact.pointA.worldSpacePoint;
            float r = glm::length(ab) - (sphereA->radius + sphereB->radius);
            contact.separationDistance = r;
            return true;
        }
    } else {
        glm::vec3 ptOnA;
        glm::vec3 ptOnB;
        const float bias = 0.001f;
        if (bezel::gjkIntersection(body, other, bias, ptOnA, ptOnB)) {
            glm::vec3 normal = ptOnB - ptOnA;
            normal = glm::normalize(normal);

            ptOnA -= normal * bias;
            ptOnB += normal * bias;

            contact.normal = normal;

            contact.pointA.worldSpacePoint = ptOnA;
            contact.pointB.worldSpacePoint = ptOnB;

            contact.pointA.modelSpacePoint =
                body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
            contact.pointB.modelSpacePoint =
                other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

            glm::vec3 ab = other->position.toGlm() - body->position.toGlm();
            float r = glm::length(ptOnA - ptOnB);
            contact.separationDistance = -r;
            return true;
        }

        bezel::gjkClosestPoints(body, other, ptOnA, ptOnB);
        contact.pointA.worldSpacePoint = ptOnA;
        contact.pointB.worldSpacePoint = ptOnB;

        contact.pointA.modelSpacePoint =
            body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
        contact.pointB.modelSpacePoint =
            other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

        glm::vec3 ab = other->position.toGlm() - body->position.toGlm();
        float r = glm::length(ptOnA - ptOnB);
        contact.separationDistance = r;
    }

    return false;
}