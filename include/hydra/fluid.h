//
// fluid.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Water and fluid simulation
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef HYDRA_FLUID_H
#define HYDRA_FLUID_H

#include "atlas/component.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <memory>

struct Fluid : GameObject {
    float waveVelocity = 0.0f;

    Fluid(Size2d extent = {0.5, 0.5},
          Color color = Color(0.0f, 0.3f, 0.5f, 1.f));
    ~Fluid() = default;

    void initialize() override;
    void update(Window &window) override;
    void render(float dt) override;

    void move(const Position3d &delta) override {
        position.x += delta.x;
        position.y += delta.y;
        position.z += delta.z;
        if (waterPlane) {
            waterPlane->move(delta);
        }
    }

    void setPosition(const Position3d &pos) override {
        position = pos;
        if (waterPlane) {
            waterPlane->setPosition(pos);
        }
    }

    void setRotation(const Rotation3d &rot) override {
        if (waterPlane) {
            waterPlane->setRotation(rot);
        }
    }

    void setExtent(const Size2d &ext) {
        extent = ext;
        if (waterPlane) {
            waterPlane->setScale({extent.width, 1.0, extent.height});
        }
    }

    void setWaveVelocity(float velocity) { waveVelocity = velocity; }

    void setWaterColor(const Color &color) {
        if (waterPlane) {
            waterPlane->material.albedo = color;
        }
    }

    bool canUseDeferredRendering() const override { return false; }

  private:
    Size2d extent;
    Position3d position;

    std::shared_ptr<CoreObject> waterPlane;
};

#endif // HYDRA_FLUID_H