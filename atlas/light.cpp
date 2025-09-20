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
    CoreObject sphere = createSphere(0.1f, 36, 18, this->color);
    sphere.setPosition(this->position);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    sphere.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(sphere);
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

PointLightConstants Light::calculateConstants() const {
    struct Entry {
        float distance, constant, linear, quadratic;
    };
    static const Entry table[] = {
        {7, 1.0f, 0.7f, 1.8f},        {13, 1.0f, 0.35f, 0.44f},
        {20, 1.0f, 0.22f, 0.20f},     {32, 1.0f, 0.14f, 0.07f},
        {50, 1.0f, 0.09f, 0.032f},    {65, 1.0f, 0.07f, 0.017f},
        {100, 1.0f, 0.045f, 0.0075f}, {160, 1.0f, 0.027f, 0.0028f},
        {200, 1.0f, 0.022f, 0.0019f}, {325, 1.0f, 0.014f, 0.0007f},
        {600, 1.0f, 0.007f, 0.0002f}, {3250, 1.0f, 0.0014f, 0.000007f},
    };

    const int n = sizeof(table) / sizeof(table[0]);

    if (distance <= table[0].distance) {
        return {distance, table[0].constant, table[0].linear,
                table[0].quadratic};
    }
    if (distance >= table[n - 1].distance) {
        return {distance, table[n - 1].constant, table[n - 1].linear,
                table[n - 1].quadratic};
    }

    for (int i = 0; i < n - 1; i++) {
        if (distance >= table[i].distance &&
            distance <= table[i + 1].distance) {
            float t = (distance - table[i].distance) /
                      (table[i + 1].distance - table[i].distance);
            float constant = table[i].constant +
                             t * (table[i + 1].constant - table[i].constant);
            float linear =
                table[i].linear + t * (table[i + 1].linear - table[i].linear);
            float quadratic = table[i].quadratic +
                              t * (table[i + 1].quadratic - table[i].quadratic);
            return {distance, constant, linear, quadratic};
        }
    }

    return {distance, 1.0f, 0.0f, 0.0f};
}

void Spotlight::createDebugObject() {
    CoreObject pyramid = createPyramid({0.1f, 0.1f, 0.1f}, this->color);
    pyramid.setPosition(this->position);
    pyramid.lookAt(this->position + this->direction);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    pyramid.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(pyramid);
}

void Spotlight::setColor(Color color) {
    this->color = color;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(color);
    }
}

void Spotlight::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}

void Spotlight::updateDebugObjectRotation() {
    if (this->debugObject != nullptr) {
        this->debugObject->setPosition(this->position);
        this->debugObject->lookAt(this->position + this->direction);
    }
}

void Spotlight::lookAt(const Position3d &target) {
    Magnitude3d newDirection = {target.x - this->position.x,
                                target.y - this->position.y,
                                target.z - this->position.z};

    this->direction = newDirection.normalized();

    updateDebugObjectRotation();
}
