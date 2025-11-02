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
#include <iostream>
#include <memory>
#include <vector>

class CompoundObject::LateCompoundRenderable : public Renderable {
  public:
    explicit LateCompoundRenderable(CompoundObject &owner) : parent(owner) {}

    void render(float dt) override { parent.renderLate(dt); }
    void initialize() override {}
    void update(Window &window) override { parent.updateLate(window); }
    void setViewMatrix(const glm::mat4 &view) override {
        parent.setLateViewMatrix(view);
    }
    void setProjectionMatrix(const glm::mat4 &projection) override {
        parent.setLateProjectionMatrix(projection);
    }
    std::optional<ShaderProgram> getShaderProgram() override {
        return parent.getLateShaderProgramInternal();
    }
    void setShader(const ShaderProgram &shader) override {
        parent.setLateShader(shader);
    }
    bool canCastShadows() const override { return parent.lateCanCastShadows(); }
    bool canUseDeferredRendering() const override { return false; }

  private:
    CompoundObject &parent;
};

Renderable *CompoundObject::getLateRenderable() {
    if (lateForwardObjects.empty()) {
        return nullptr;
    }
    if (!lateRenderableProxy) {
        lateRenderableProxy = std::make_unique<LateCompoundRenderable>(*this);
    }
    return lateRenderableProxy.get();
}

void CompoundObject::initialize() {
    init();
    for (auto &component : components) {
        component->init();
    }

    if (!lateForwardObjects.empty() && !lateRenderableRegistered) {
        if (!lateRenderableProxy) {
            lateRenderableProxy =
                std::make_unique<LateCompoundRenderable>(*this);
        }
        if (Window::mainWindow != nullptr) {
            Window::mainWindow->addLateForwardObject(lateRenderableProxy.get());
            lateRenderableRegistered = true;
        }
    }
}

void CompoundObject::render(float dt) {
    if (originalPositions.empty()) {
        for (const auto &obj : objects) {
            originalPositions.push_back(obj->getPosition());
        }
    }
    if (changedPosition) {
        for (size_t i = 0; i < objects.size(); ++i) {
            objects[i]->setPosition(position + originalPositions[i]);
        }
        changedPosition = false;
    }
    for (auto &component : components) {
        component->update(dt);
    }
    for (auto &obj : objects) {
        if (obj != nullptr && obj->renderLateForward) {
            continue;
        }
        obj->render(dt);
    }
}

void CompoundObject::renderLate(float dt) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
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
    if (!objects.empty()) {
        auto shader = objects[0]->getShaderProgram();
        if (shader.has_value()) {
            return shader;
        }
    }
    return getLateShaderProgramInternal();
}

void CompoundObject::setShader(const ShaderProgram &shader) {
    for (auto &obj : objects) {
        obj->setShader(shader);
    }
}

Position3d CompoundObject::getPosition() const {
    if (objects.empty()) {
        if (!lateForwardObjects.empty() && lateForwardObjects[0] != nullptr) {
            return lateForwardObjects[0]->getPosition();
        }
        return {0, 0, 0};
    }
    return objects[0]->getPosition();
}

Size3d CompoundObject::getScale() const {
    if (objects.empty()) {
        if (!lateForwardObjects.empty() && lateForwardObjects[0] != nullptr) {
            return lateForwardObjects[0]->getScale();
        }
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

        if (changedPosition) {
            std::cout << "Updating position of compound object child\n";
            obj->setPosition(position + this->position);
            changedPosition = false;
        }
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
    this->position = newPosition;
    changedPosition = true;
}

void CompoundObject::move(const Position3d &deltaPosition) {
    this->position += deltaPosition;
    changedPosition = true;
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

void CompoundObject::updateLate(Window &window) { (void)window; }

void CompoundObject::setLateViewMatrix(const glm::mat4 &view) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setViewMatrix(view);
    }
}

void CompoundObject::setLateProjectionMatrix(const glm::mat4 &projection) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setProjectionMatrix(projection);
    }
}

std::optional<ShaderProgram> CompoundObject::getLateShaderProgramInternal() {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        auto program = obj->getShaderProgram();
        if (program.has_value()) {
            return program;
        }
    }
    return std::nullopt;
}

void CompoundObject::setLateShader(const ShaderProgram &shader) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setShader(shader);
    }
}

bool CompoundObject::lateCanCastShadows() const {
    for (auto *obj : lateForwardObjects) {
        if (obj != nullptr && obj->canCastShadows()) {
            return true;
        }
    }
    return false;
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