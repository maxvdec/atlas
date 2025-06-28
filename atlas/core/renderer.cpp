/*
 renderer.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core rendering processes
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"

void Renderer::registerObject(CoreObject *object, RenderingFn dispatcher) {
    object->id = static_cast<int>(this->registeredObjects.size());
    this->registeredObjects.push_back(object);
    this->dispactchers.push_back(dispatcher);
}

void Renderer::dispatchAll() {
    for (size_t i = 0; i < this->registeredObjects.size(); ++i) {
        if (this->dispactchers[i]) {
            this->dispactchers[i](this->registeredObjects[i]);
        }
    }
}
