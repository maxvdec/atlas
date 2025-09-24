/*
 shape.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Simple shape functions and defintions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_SHAPE_H
#define ATLAS_SHAPE_H

#include <glm/glm.hpp>

class Shape {
  public:
    enum class ShapeType { Sphere };

    virtual ShapeType getType() const = 0;

    virtual glm::vec3 getCenterOfMass() const { return centerOfMass; }

    virtual glm::mat3 getInertiaTensor() const = 0;

  protected:
    glm::vec3 centerOfMass = {0.0f, 0.0f, 0.0f};
};

class Sphere : public Shape {
  public:
    Sphere(float radius);
    ShapeType getType() const override { return ShapeType::Sphere; }

    glm::mat3 getInertiaTensor() const override;

    float radius;
};

#endif // ATLAS_SHAPE_H
