//
// bezel.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Common Bezel API definition
// Copyright (c) 2025 maxvdec
//

#ifndef BEZEL_H
#define BEZEL_H

#include "atlas/units.h"
class Window;

class Body {
  private:
    float mass = 0;

  public:
    Position3d position;
    Rotation3d orientation;

    void applyMass(float targetMass) {
        if (targetMass == INFINITY) {
            this->mass = 0;
        } else {
            this->mass = targetMass;
        }
    }

    void update(Window &window);
};

#endif
