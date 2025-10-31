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

#include "atlas/component.h"
#include "atlas/input.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <array>
#include <memory>
#include <vector>

class WorleyNoise3D {
  public:
    WorleyNoise3D(int frequency, int numberOfDivisions);
    float getValue(float x, float y, float z) const;

    Id get3dTexture(int res) const;
    Id getDetailTexture(int res) const;
    Id get3dTextureAtAllChannels(int res) const;

  private:
    int frequency;
    int numberOfDivisions;
    std::vector<glm::vec3> featurePoints;

    void generateFeaturePoints();
    float getWorleyNoise(float x, float y, float z, int octave) const;
    std::vector<float> getClosestDistances(float x, float y, float z,
                                           int count) const;
    glm::ivec3 getGridCell(float x, float y, float z) const;
    int getCellIndex(int cx, int cy, int cz) const;

    Id createTexture3d(const std::vector<float> &data, int res, GLenum format,
                       GLenum internalFormat) const;
};

class Clouds {
  public:
    Clouds(int frequency, int divisions) : worleyNoise(frequency, divisions) {};

    Id getCloudTexture(int res) const;

    Position3d position = {0.0f, 5.0f, 0.0f};
    Size3d size = {10.0f, 3.0f, 10.0f};
    float scale = 1.5f;
    Position3d offset = {0.0f, 0.0f, 0.0f};
    float density = 0.45f;
    float densityMultiplier = 1.5f;
    float absorption = 1.1f;
    float scattering = 0.85f;
    float phase = 0.55f;
    float clusterStrength = 0.5f;
    int primaryStepCount = 12;
    int lightStepCount = 6;
    float lightStepMultiplier = 1.6f;
    float minStepLength = 0.05f;
    Magnitude3d wind = {0.02f, 0.0f, 0.01f};

  private:
    WorleyNoise3D worleyNoise = WorleyNoise3D(4, 6);

    mutable Id cachedTextureId = 0;
    mutable int cachedResolution = 0;
};

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

    std::shared_ptr<Clouds> clouds = nullptr;

    std::array<Color, 6> getSkyboxColors() const;
    Cubemap createSkyCubemap(int size = 256) const;
    void updateSkyCubemap(Cubemap &cubemap) const;

    Color sunColor = Color(1.0, 0.95, 0.8, 1.0);
    Color moonColor = Color(0.5, 0.5, 0.8, 1.0);

    float sunSize = 1.0f;
    float moonSize = 1.0f;

    float sunTintStrength = 0.3f;
    float moonTintStrength = 0.8f;
    float starIntensity = 3.0f;

    inline bool isDaytime() const {
        Magnitude3d sunDir = getSunAngle();
        return sunDir.y > 0.0f;
    }

    inline void setTime(float hours) {
        if (hours > 0 && hours < 24)
            timeOfDay = hours;
    }

    inline void addClouds(int frequency = 4, int divisions = 6) {
        if (!clouds) {
            clouds = std::make_shared<Clouds>(Clouds(frequency, divisions));
        }
    }

    bool cycle = false;

  private:
    mutable float lastSkyboxUpdateTime = -1.0f;
    mutable bool skyboxCacheValid = false;
    mutable std::array<Color, 6> lastSkyboxColors = {};
};

#endif // HYDRA_ATMOSPHERE_H
