/*
 shape.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Simple shape functions and defintions
 Copyright (c) 2025 maxvdec
*/

#ifndef BEZEL_SHAPE_H
#define BEZEL_SHAPE_H

#include "bezel/bounds.h"
#include <glm/glm.hpp>

class Shape {
  public:
    enum class ShapeType { Sphere };

    virtual ShapeType getType() const = 0;

    virtual glm::vec3 getCenterOfMass() const { return centerOfMass; }

    virtual glm::mat3 getInertiaTensor() const = 0;

    virtual Bounds getBounds(const glm::vec3 &pos,
                             const glm::quat &orientation) const = 0;
    virtual Bounds getBounds() const = 0;

  protected:
    glm::vec3 centerOfMass = {0.0f, 0.0f, 0.0f};
};

class Sphere : public Shape {
  public:
    explicit Sphere(float radius);
    ShapeType getType() const override { return ShapeType::Sphere; }

    glm::mat3 getInertiaTensor() const override;

    Bounds getBounds(const glm::vec3 &pos,
                     const glm::quat &orientation) const override;
    Bounds getBounds() const override;

    float radius;
};

namespace bezel {
bool raySphere(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
               const glm::vec3 &sphereCenter, const float sphereRadius,
               float &t1, float &t2);
bool sphereToSphereDynamic(const Sphere *sphereA, const Sphere *sphereB,
                           const glm::vec3 &posA, const glm::vec3 &posB,
                           const glm::vec3 &velA, const glm::vec3 &velB,
                           const float dt, glm::vec3 &pointOnA,
                           glm::vec3 &pointOnB, float &toi);
} // namespace bezel

#endif // BEZEL_SHAPE_H
