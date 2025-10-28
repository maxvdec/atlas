//
// mainFluid.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Main file for fluid simulations
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/camera.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include "hydra/fluid.h"
#include <iostream>

class FluidMainScene : public Scene {
    Camera camera;
    Fluid fluid;

  public:
    void update(Window &window) override {}
    void initialize(Window &window) override {
        camera = Camera();
        camera.setPosition({5.0f, 5.0f, 13.0f}); // View from an angle
        camera.lookAt({5.0f, 5.0f, 5.0f});       // Look at center of cube
        window.setCamera(&camera);
        window.addObject(&fluid);

        Cubemap cubemap = Cubemap::fromColor(Color::black(), 1.0f);
        this->setSkybox(Skybox::create(cubemap, window));
    }
};

int main() {
    Window window({.title = "Fluid Simulation",
                   .width = 1600,
                   .height = 1200,
                   .mouseCaptured = false});
    FluidMainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}