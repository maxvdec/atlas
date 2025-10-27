/*
 atmosphere.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Daylight and atmospheric effects
 Copyright (c) 2025 Max Van den Eynde
*/

#ifndef HYDRA_ATMOSPHERE_H
#define HYDRA_ATMOSPHERE_H

#include "atlas/units.h"
#include <optional>

class Atmosphere {
  public:
    bool enabled = false;
    float secondsPerHour = 60.f;
    float timeOfDay;

    void update(float dt);
    void enable() { enabled = true; }

    float getNormalizedTime() const;

    Magnitude3d getSunAngle() const;
    Magnitude3d getMoonAngle() const;

    float getLightIntensity() const;
    Color getLightColor() const;
};

#endif // HYDRA_ATMOSPHERE_H
