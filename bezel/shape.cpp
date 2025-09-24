/*
 shape.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shape functions
 Copyright (c) 2025 maxvdec
*/

#include "bezel/shape.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

Sphere::Sphere(float radius) : radius(radius) {
    this->centerOfMass = {0.0f, 0.0f, 0.0f};
}

glm::mat3 Sphere::getInertiaTensor() const {
    float coeff = 2.0f * radius * radius / 5.0f;
    return glm::mat3(coeff, 0.0f, 0.0f, 0.0f, coeff, 0.0f, 0.0f, 0.0f, coeff);
}

bool bezel::raySphere(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                      const glm::vec3 &sphereCenter, const float sphereRadius,
                      float &t1, float &t2) {
    glm::vec3 m = sphereCenter - rayOrigin;
    float a = glm::dot(rayDirection, rayDirection);
    float b = glm::dot(m, rayDirection);
    float c = glm::dot(m, m) - sphereRadius * sphereRadius;

    float discriminant = b * b - a * c;
    float invA = 1.0f / a;

    if (discriminant < 0) {
        return false;
    }

    float deltaRoot = std::sqrt(discriminant);
    t1 = invA * (b - deltaRoot);
    t2 = invA * (b + deltaRoot);

    return true;
}

bool bezel::sphereToSphereDynamic(const Sphere *sphereA, const Sphere *sphereB,
                                  const glm::vec3 &posA, const glm::vec3 &posB,
                                  const glm::vec3 &velA, const glm::vec3 &velB,
                                  const float dt, glm::vec3 &pointOnA,
                                  glm::vec3 &pointOnB, float &toi) {
    glm::vec3 relativeVelocity = velA - velB;

    const glm::vec3 startPointA = posA;
    const glm::vec3 endPointA = posA + relativeVelocity * dt;
    glm::vec3 rayDir = endPointA - startPointA;

    float t0 = 0;
    float t1 = 0;
    if (glm::length2(rayDir) < 1e-8f) {
        glm::vec3 ab = posB - posA;
        float radius = sphereA->radius + sphereB->radius + 0.001f;
        if (glm::length2(ab) > radius * radius) {
            return false;
        }
    } else if (!raySphere(posA, rayDir, posB, sphereA->radius + sphereB->radius,
                          t0, t1)) {
        return false;
    }

    t0 *= dt;
    t1 *= dt;

    if (t1 < 0.0f) {
        return false;
    }

    toi = (t0 < 0.0f) ? 0.0f : t0;

    if (toi > dt) {
        return false;
    }

    glm::vec3 newPosA = posA + velA * toi;
    glm::vec3 newPosB = posB + velB * toi;
    glm::vec3 ab = newPosB - newPosA;
    ab = glm::normalize(ab);

    pointOnA = newPosA + ab * sphereA->radius;
    pointOnB = newPosB - ab * sphereB->radius;
    return true;
}
