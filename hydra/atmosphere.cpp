/*
 atmosphere.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Implementation of the atmosphere model.
 Copyright (c) 2025 Max Van den Eynde
*/

#include "atlas/units.h"
#include <cmath>
#include <hydra/atmosphere.h>
#include <iostream>

void Atmosphere::update(float dt) {
    if (!enabled)
        return;

    timeOfDay += (dt / secondsPerHour);
    if (timeOfDay >= 24.0f) {
        timeOfDay -= 24.0f;
    }
}

Magnitude3d Atmosphere::getSunAngle() const {
    float sunAngle = (timeOfDay / 24.0f) * 360.f - 90.f;
    Magnitude3d sunDir = Magnitude3d(std::cos(glm::radians(sunAngle)),
                                     std::sin(glm::radians(sunAngle)), 0.0f);
    return sunDir;
}

Magnitude3d Atmosphere::getMoonAngle() const {
    float moonAngle = (timeOfDay / 24.0f) * 360.f - 90.f;
    Magnitude3d moonDir = Magnitude3d(std::cos(glm::radians(moonAngle)),
                                      std::sin(glm::radians(moonAngle)), 0.0f);
    return moonDir * -1.0f;
}

float Atmosphere::getLightIntensity() const {
    Magnitude3d sunDir = getSunAngle();
    float daylight = glm::clamp((float)sunDir.y * 2.0f, 0.0f, 1.0f);
    return glm::mix(0.05f, 1.0f, daylight);
}

Color Atmosphere::getLightColor() const {
    Magnitude3d sunAngle = getSunAngle();
    float daylight = glm::clamp((float)sunAngle.y * 2.0f, 0.0f, 1.0f);
    return Color::mix(Color(0.1f, 0.1f, 0.3f), Color(1.0f, 0.95f, 0.8f),
                      daylight);
}
