/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting functions and solutions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.hpp"
#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/units.hpp"
#include "atlas/window.hpp"
#include <glm/glm.hpp>
#include <iostream>

Light::Light(Position3d position, Color color, LightType type, Scene *scene,
             float intensity)
    : position(position), color(color) {

    this->debugObject = generateCubeObject(position, Size3d(0.1f, 0.1f, 0.1f));
    this->intensity = intensity;
    for (int i = 0; i < debugObject.vertices.size(); ++i) {
        debugObject.setVertexColor(i, color);
    }

    debugObject.fragmentShader =
        CoreShader(NORMAL_FRAG, CoreShaderType::Fragment);

    debugObject.hide();
    debugObject.initialize();
    this->type = type;
    if (Window::current_window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    if (Window::current_window->currentScene == nullptr) {
        return;
    }
    Window::current_window->currentScene->useLight(this);
}

void Light::debugLight() { this->debugObject.show(); }

const std::vector<std::tuple<float, float, float, float>> attenuationTable = {
    {7, 1.0f, 0.7f, 1.8f},        {13, 1.0f, 0.35f, 0.44f},
    {20, 1.0f, 0.22f, 0.20f},     {32, 1.0f, 0.14f, 0.07f},
    {50, 1.0f, 0.09f, 0.032f},    {65, 1.0f, 0.07f, 0.017f},
    {100, 1.0f, 0.045f, 0.0075f}, {160, 1.0f, 0.027f, 0.0028f},
    {200, 1.0f, 0.022f, 0.0019f}, {325, 1.0f, 0.014f, 0.0007f},
    {600, 1.0f, 0.007f, 0.0002f}, {3250, 1.0f, 0.0014f, 0.000007f}};

Attenuation getAttenuationForDistance(float distance) {
    if (distance <= std::get<0>(attenuationTable.front())) {
        return {std::get<1>(attenuationTable.front()),
                std::get<2>(attenuationTable.front()),
                std::get<3>(attenuationTable.front())};
    }
    if (distance >= std::get<0>(attenuationTable.back())) {
        return {std::get<1>(attenuationTable.back()),
                std::get<2>(attenuationTable.back()),
                std::get<3>(attenuationTable.back())};
    }

    for (size_t i = 0; i < attenuationTable.size() - 1; ++i) {
        float d0 = std::get<0>(attenuationTable[i]);
        float d1 = std::get<0>(attenuationTable[i + 1]);

        if (distance >= d0 && distance <= d1) {
            float t = (distance - d0) / (d1 - d0);

            float constant = (1.0f - t) * std::get<1>(attenuationTable[i]) +
                             t * std::get<1>(attenuationTable[i + 1]);
            float linear = (1.0f - t) * std::get<2>(attenuationTable[i]) +
                           t * std::get<2>(attenuationTable[i + 1]);
            float quad = (1.0f - t) * std::get<3>(attenuationTable[i]) +
                         t * std::get<3>(attenuationTable[i + 1]);

            return {constant, linear, quad};
        }
    }

    return {1.0f, 0.045f, 0.0075f};
}
