/*
 light.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Light header and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_LIGHT_HPP
#define ATLAS_LIGHT_HPP

#include "atlas/core/rendering.hpp"
#include "atlas/material.hpp"
#include "atlas/units.hpp"
#include <glm/glm.hpp>

enum class LightType {
    Point,
    Directional,
    SpotLight,
    None,
};

#define MAX_LIGHTS 10

struct Scene;

class Light {
  public:
    Position3d position;
    Color color;
    Color ambientColor = Color(0.2f, 0.2f, 0.2f, 1.0f);

    Light(Position3d position = Position3d(0.0f, 0.0f, 0.0f),
          Color color = Color(1.0f, 1.0f, 1.0f),
          LightType type = LightType::None, Scene *scene = nullptr);

    void debugLight();
    CoreObject debugObject;
    float intensity = 5.f;

    Material material = Material();

    LightType type;
};

class DirectionalLight : public Light {
  public:
    Position3d direction;

    DirectionalLight(Position3d direction = Position3d(0.0f, -1.0f, 0.0f),
                     Color color = Color(1.0f, 1.0f, 1.0f),
                     Scene *scene = nullptr)
        : Light(Position3d(0.0f, 0.0f, 0.0f), color, LightType::Directional,
                scene),
          direction(direction) {};
};

struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

Attenuation getAttenuationForDistance(float distance);

class PointLight : public Light {
  public:
    float max_distance = 100.0f;
    Attenuation attenuation = {1.0f, 0.09f, 0.032f};

    inline void changeMaxDistance(float distance) {
        this->max_distance = distance;
        this->attenuation = getAttenuationForDistance(distance);
    }

    PointLight(Position3d position = Position3d(0.0f, 0.0f, 0.0f),
               Color color = Color(1.0f, 1.0f, 1.0f), Scene *scene = nullptr)
        : Light(position, color, LightType::Point, scene) {};
};

class SpotLight : public Light {
  public:
    Position3d direction;
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));
    Attenuation attenuation = {1.0f, 0.09f, 0.032f};
    float max_distance = 100.0f;

    inline void changeMaxDistance(float distance) {
        this->max_distance = distance;
        this->attenuation = getAttenuationForDistance(distance);
    }

    SpotLight(Position3d position = Position3d(0.0f, 0.0f, 0.0f),
              Position3d direction = Position3d(0.0f, -1.0f, 0.0f),
              Color color = Color(1.0f, 1.0f, 1.0f), Scene *scene = nullptr)
        : Light(position, color, LightType::SpotLight, scene),
          direction(direction) {};
};

#endif // ATLAS_LIGHT_HPP
