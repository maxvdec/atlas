//
// constraint.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Contraint base class and definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef BEZEL_CONSTRAINT_H
#define BEZEL_CONSTRAINT_H

#include "bezel/abstract.h"
#include "bezel/body.h"
#include <glm/glm.hpp>
#include <memory>

class Constraint {
  public:
    virtual void preSolve(float dt) {};
    virtual void solve() {};
    virtual void postSolve() {};

  protected:
    bezel::matMN getInverseMassMatrix() const;
    bezel::vecN getVelocities() const;
    void applyImpulses(const bezel::vecN &impulses);

  public:
    std::shared_ptr<Body> bodyA;
    std::shared_ptr<Body> bodyB;

    glm::vec3 anchorA;
    glm::vec3 axisA;

    glm::vec3 anchorB;
    glm::vec3 axisB;
};

namespace bezel {
vecN lcpGaussSeidel(const matN &A, const vecN &b);
}

#endif // BEZEL_CONSTRAINT_H