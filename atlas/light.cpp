/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting helper functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/window.h"

void Light::createDebugObject() {
    CoreObject cube = createBox({0.1f, 0.1f, 0.1f}, this->color);
    cube.setPosition(this->position);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    cube.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(cube);
}

void Light::setColor(Color color) {
    this->color = color;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(color);
    }
}

void Light::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}
