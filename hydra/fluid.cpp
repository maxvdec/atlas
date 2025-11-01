//
// fluid.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Fluid implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "hydra/fluid.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <memory>

Fluid::Fluid(Size2d extent, Color color) : GameObject() {
    this->waterPlane = std::make_shared<CoreObject>(createPlane(extent, color));
}

void Fluid::initialize() {
    if (waterPlane) {
        waterPlane->initialize();
        waterPlane->setPosition(position);
        waterPlane->setScale({extent.width, 1.0, extent.height});
        waterPlane->setShader(ShaderProgram::fromDefaultShaders(
            AtlasVertexShader::Fluid, AtlasFragmentShader::Fluid));
    }
}

void Fluid::render(float dt) {
    if (waterPlane) {
        waterPlane->render(dt);
    }
}

void Fluid::update(Window &window) {}