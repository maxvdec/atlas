//
// compound.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Compound Object implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <vector>

void CompoundObject::initialize() {
    init();
    for (auto &component : components) {
        component->init();
    }
}

void CompoundObject::render(float dt) {
    for (auto &component : components) {
        component->update(dt);
    }
    for (auto &obj : objects) {
        obj->render(dt);
    }
}

void CompoundObject::setViewMatrix(const glm::mat4 &view) {
    for (auto &obj : objects) {
        obj->setViewMatrix(view);
    }
}

void CompoundObject::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &obj : objects) {
        obj->setProjectionMatrix(projection);
    }
}

std::optional<ShaderProgram> CompoundObject::getShaderProgram() {
    if (objects.empty()) {
        return std::nullopt;
    }
    return objects[0]->getShaderProgram();
}

void CompoundObject::setShader(const ShaderProgram &shader) {
    for (auto &obj : objects) {
        obj->setShader(shader);
    }
}

Position3d CompoundObject::getPosition() const {
    if (objects.empty()) {
        return {0, 0, 0};
    }
    return objects[0]->getPosition();
}

Size3d CompoundObject::getScale() const {
    if (objects.empty()) {
        return {1, 1, 1};
    }
    return objects[0]->getScale();
}

void CompoundObject::update(Window &window) {
    updateObjects(window);
    for (auto &obj : objects) {
        if (obj->body == nullptr)
            continue;
        obj->body->update(window);
        Position3d position = obj->body->position;
        Rotation3d rotation = Rotation3d::fromGlmQuat(obj->body->orientation);

        obj->setPosition(position);
        obj->setRotation(rotation);
        obj->setScale(this->getScale());
        obj->updateModelMatrix();
    }
}

bool CompoundObject::canCastShadows() const {
    for (auto &obj : objects) {
        if (obj->canCastShadows()) {
            return true;
        }
    }
    return false;
}

void CompoundObject::setPosition(const Position3d &newPosition) {
    for (auto &obj : objects) {
        obj->move(newPosition - obj->getPosition());
    }
}

void CompoundObject::move(const Position3d &deltaPosition) {
    for (auto &obj : objects) {
        obj->move(deltaPosition);
    }
}

void CompoundObject::setRotation(const Rotation3d &newRotation) {
    for (auto &obj : objects) {
        obj->setRotation(newRotation);
    }
}

void CompoundObject::lookAt(const Position3d &target, const Normal3d &up) {
    for (auto &obj : objects) {
        obj->lookAt(target, up);
    }
}

void CompoundObject::rotate(const Rotation3d &deltaRotation) {
    for (auto &obj : objects) {
        obj->rotate(deltaRotation);
    }
}

void CompoundObject::setScale(const Scale3d &newScale) {
    for (auto &obj : objects) {
        obj->setScale(newScale);
    }
}

void CompoundObject::hide() {
    for (auto &obj : objects) {
        obj->hide();
    }
}

void CompoundObject::show() {
    for (auto &obj : objects) {
        obj->show();
    }
}

void CompoundObject::setupPhysics(Body body) {
    for (auto &obj : objects) {
        if (obj->body == nullptr) {
            obj->setupPhysics(body);
        }
    }
}

std::vector<CoreVertex> CompoundObject::getVertices() const {
    std::vector<CoreVertex> allVertices;
    for (const auto &obj : objects) {
        std::vector<CoreVertex> objVertices = obj->getVertices();
        allVertices.insert(allVertices.end(), objVertices.begin(),
                           objVertices.end());
    }
    return allVertices;
}

Window *Component::getWindow() { return Window::mainWindow; }

void UIView::setViewMatrix(const glm::mat4 &view) {
    for (auto &obj : children) {
        obj->setViewMatrix(view);
    }
}

void UIView::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &obj : children) {
        obj->setProjectionMatrix(projection);
    }
}

void UIView::render(float dt) {
    for (auto &component : components) {
        component->update(dt);
    }
    for (auto &obj : children) {
        obj->render(dt);
    }
}