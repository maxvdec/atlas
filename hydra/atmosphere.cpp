/*
 atmosphere.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Implementation of the atmosphere model.
 Copyright (c) 2025 Max Van den Eynde
*/

#include <hydra/atmosphere.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "atlas/units.h"

namespace {
struct SkyKeyframe {
    float time;
    std::array<Color, 6> colors;
};

std::array<Color, 6> blendKeyframeColors(const std::array<Color, 6> &a,
                                         const std::array<Color, 6> &b,
                                         float t) {
    std::array<Color, 6> blended{};
    float ratio = std::clamp(t, 0.0f, 1.0f);
    for (size_t i = 0; i < blended.size(); ++i) {
        blended[i] = Color::mix(a[i], b[i], ratio);
    }
    return blended;
}

const std::array<SkyKeyframe, 7> &skyKeyframes() {
    static const std::array<SkyKeyframe, 7> frames = {
        {{0.0f,
          {Color::fromHex(0x04081A), Color::fromHex(0x04081A),
           Color::fromHex(0x020310), Color::fromHex(0x080910),
           Color::fromHex(0x030A1C), Color::fromHex(0x030A1C)}},
         {0.18f,
          {Color::fromHex(0xF7B486), Color::fromHex(0x1B2F5C),
           Color::fromHex(0x142447), Color::fromHex(0x201924),
           Color::fromHex(0x2A4C7C), Color::fromHex(0xD98463)}},
         {0.25f,
          {Color::fromHex(0xFDBA76), Color::fromHex(0x355DA0),
           Color::fromHex(0x2C5AA2), Color::fromHex(0xFADFC2),
           Color::fromHex(0x4D7FC9), Color::fromHex(0xFF9D70)}},
         {0.50f,
          {Color::fromHex(0x79C3FF), Color::fromHex(0x7EC8FF),
           Color::fromHex(0x2D6BD6), Color::fromHex(0xF8EEDC),
           Color::fromHex(0x7FCBFF), Color::fromHex(0x78C1FF)}},
         {0.72f,
          {Color::fromHex(0x4762A8), Color::fromHex(0xFF8856),
           Color::fromHex(0x32549A), Color::fromHex(0xF6C3A3),
           Color::fromHex(0x5F73B8), Color::fromHex(0xFF7442)}},
         {0.82f,
          {Color::fromHex(0x1E2C57), Color::fromHex(0xFE9068),
           Color::fromHex(0x1B2F56), Color::fromHex(0x1F1B28),
           Color::fromHex(0x273C6D), Color::fromHex(0xE06A4C)}},
         {1.0f,
          {Color::fromHex(0x04081A), Color::fromHex(0x04081A),
           Color::fromHex(0x020310), Color::fromHex(0x080910),
           Color::fromHex(0x030A1C), Color::fromHex(0x030A1C)}}}};
    return frames;
}
} // namespace

void Atmosphere::update(float dt) {
    if (!enabled)
        return;

    timeOfDay += (dt / secondsPerHour);
    if (timeOfDay >= 24.0f) {
        timeOfDay -= 24.0f;
    }
}

float Atmosphere::getNormalizedTime() const {
    if (std::isnan(timeOfDay) || std::isinf(timeOfDay)) {
        return 0.0f;
    }
    float wrappedHours = std::fmod(timeOfDay, 24.0f);
    if (wrappedHours < 0.0f) {
        wrappedHours += 24.0f;
    }
    return wrappedHours / 24.0f;
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
    return glm::mix(0.01f, 1.0f, daylight);
}

Color Atmosphere::getLightColor() const {
    Magnitude3d sunAngle = getSunAngle();
    float daylight = glm::clamp((float)sunAngle.y * 2.0f, 0.0f, 1.0f);
    return Color::mix(Color{0.05, 0.07, 0.18, 1.0}, Color{1.0, 0.95, 0.8, 1.0},
                      daylight);
}

std::array<Color, 6> Atmosphere::getSkyboxColors() const {
    float normalized = getNormalizedTime();
    const auto &frames = skyKeyframes();

    for (size_t i = 1; i < frames.size(); ++i) {
        if (normalized <= frames[i].time) {
            float span = frames[i].time - frames[i - 1].time;
            float t =
                span > 0.0f ? (normalized - frames[i - 1].time) / span : 0.0f;
            return blendKeyframeColors(frames[i - 1].colors, frames[i].colors,
                                       t);
        }
    }

    return frames.back().colors;
}

Cubemap Atmosphere::createSkyCubemap(int size) const {
    int cubemapSize = std::max(1, size);
    auto colors = getSkyboxColors();
    Cubemap cubemap = Cubemap::fromColors(colors, cubemapSize);
    lastSkyboxColors = colors;
    skyboxCacheValid = true;
    lastSkyboxUpdateTime = getNormalizedTime();
    return cubemap;
}

void Atmosphere::updateSkyCubemap(Cubemap &cubemap) const {
    if (cubemap.id == 0 || cubemap.creationData.width <= 0) {
        return;
    }

    float normalized = getNormalizedTime();
    if (lastSkyboxUpdateTime >= 0.0f) {
        float delta = std::fabs(normalized - lastSkyboxUpdateTime);
        delta = std::min(delta, 1.0f - delta);
        constexpr float minDelta = 1.0f / 360.0f; // Update roughly every 4 min
        if (delta < minDelta) {
            return;
        }
    }

    auto colors = getSkyboxColors();

    if (skyboxCacheValid) {
        double maxComponentDelta = 0.0;
        for (size_t face = 0; face < colors.size(); ++face) {
            const Color &newColor = colors[face];
            const Color &oldColor = lastSkyboxColors[face];
            maxComponentDelta =
                std::max(maxComponentDelta, std::fabs(newColor.r - oldColor.r));
            maxComponentDelta =
                std::max(maxComponentDelta, std::fabs(newColor.g - oldColor.g));
            maxComponentDelta =
                std::max(maxComponentDelta, std::fabs(newColor.b - oldColor.b));
        }
        constexpr double colorThreshold = 1e-3;
        if (maxComponentDelta < colorThreshold) {
            lastSkyboxUpdateTime = normalized;
            return;
        }
    }

    cubemap.updateWithColors(colors);
    lastSkyboxColors = colors;
    skyboxCacheValid = true;
    lastSkyboxUpdateTime = normalized;
}
