/*
 renderer.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core rendering processes
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include <glad/glad.h>
#include <iostream>

void Renderer::registerObject(CoreObject *object, RenderingFn dispatcher) {
    if (object == nullptr || object->program.has_value() == false) {
        std::cerr << "Error: Attempting to register a null object."
                  << std::endl;
        return;
    }
    object->id = static_cast<int>(this->registeredObjects.size() + 1);
    std::cout << "Registering object with ID: " << object->id << std::endl;
    this->registeredObjects.push_back(object);
    this->dispatchers.push_back(dispatcher);
}

void Renderer::dispatchAll() {
    for (size_t i = 0; i < this->registeredObjects.size(); ++i) {
        if (this->registeredObjects[i] == nullptr) {
            std::cerr
                << "Warning: Attempting to dispatch a null object at index "
                << i << std::endl;
            continue;
        }

        if (this->registeredObjects[i]->hidden) {
            continue;
        }
        this->dispatchers[i](this->registeredObjects[i]);
    }
}

void checkGLError(const std::string &operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during " << operation << ": " << error
                  << std::endl;
        throw std::runtime_error("OpenGL error occurred");
    }
}
