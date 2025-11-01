//
// scene.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Scene implementation for the Atlas Engine
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include <algorithm>
#include <array>

void Scene::updateScene(float dt) {
    atmosphere.update(dt);
    if (atmosphere.isEnabled()) {
        constexpr int dynamicSkyResolution = 128;

        if (this->skybox == nullptr ||
            this->skybox->cubemap.creationData.width != dynamicSkyResolution) {
            Cubemap dynamicCubemap =
                atmosphere.createSkyCubemap(dynamicSkyResolution);
            this->setSkybox(
                Skybox::create(dynamicCubemap, *(Window::mainWindow)));
            if (automaticAmbient) {
                updateAutomaticAmbientFromSkybox();
                float lightIntensity =
                    std::clamp(atmosphere.getLightIntensity(), 0.02f, 1.0f);
                automaticAmbientIntensity = lightIntensity * 0.25f;
            }
        } else {
            atmosphere.updateSkyCubemap(this->skybox->cubemap);
            if (automaticAmbient) {
                updateAutomaticAmbientFromSkybox();
                float lightIntensity =
                    std::clamp(atmosphere.getLightIntensity(), 0.02f, 1.0f);
                automaticAmbientIntensity = lightIntensity * 0.25f;
            }
        }
    } else if (this->skybox == nullptr) {
        static const std::array<Color, 6> noonSkyColors = {
            Color::fromHex(0x7FC1FF), // +X east horizon
            Color::fromHex(0x89CBFF), // -X west horizon
            Color::fromHex(0x2F62D5), // +Y zenith
            Color::fromHex(0xF6E9D2), // -Y subtle ground glow
            Color::fromHex(0x85CCFF), // +Z north horizon
            Color::fromHex(0x80C6FF)  // -Z south horizon
        };

        Cubemap defaultCubemap = Cubemap::fromColors(noonSkyColors, 1024);
        this->setSkybox(Skybox::create(defaultCubemap, *(Window::mainWindow)));
    }
}