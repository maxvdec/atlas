//
// layout.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Layout definitions and calculations
// Copyright (c) 2026 Max Van den Eynde
//

#include "graphite/layout.h"
#include "atlas/component.h"

void Column::addChild(UIObject *child) {
    children.push_back(child);
    recalculatePositions();
}

void Column::setChildren(const std::vector<UIObject *> &newChildren) {
    children = newChildren;
    recalculatePositions();
}

void Column::setViewMatrix(const glm::mat4 &view) {
    for (auto &child : children) {
        child->setViewMatrix(view);
    }
}

void Column::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        child->setProjectionMatrix(projection);
    }
}

void Column::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    for (auto &child : children) {
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Column::recalculatePositions() {
    float currentY = this->position.y + padding.height;

    for (auto &child : children) {
        Position2d pos;

        pos.x = this->position.x + padding.width;
        pos.y = currentY;

        child->setScreenPosition(pos);

        currentY += child->getSize().height + spacing;
    }
}