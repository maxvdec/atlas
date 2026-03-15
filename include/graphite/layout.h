//
// layout.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Layout structures for Graphite UI
// Copyright (c) 2026 Max Van den Eynde
//

#ifndef GRAPHITE_LAYOUT_H
#define GRAPHITE_LAYOUT_H

#include "atlas/units.h"
#include <atlas/component.h>
#include <vector>

class Column : public UIObject {
  public:
    Column(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }
    Column(const std::vector<UIObject *> &children, float spacing = 0.0f,
           Size2d padding = {.width = 0.0f, .height = 0.0f},
           Position2d pos = {.x = 0, .y = 0})
        : spacing(spacing), padding(padding), children(children),
          position(pos) {
        recalculatePositions();
    }
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override;

    float spacing = 0.0f;
    Size2d maxSize{.width = 0.0f, .height = 0.0f};
    Size2d padding{.width = 0.0f, .height = 0.0f};
    std::vector<UIObject *> children;
    Position2d position;

    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    void addChild(UIObject *child);
    void setChildren(const std::vector<UIObject *> &newChildren);

    Size2d getSize() const override {
        float width = 0.0f;
        float totalHeight =
            (padding.height * 2) + (spacing * (children.size() - 1));
        for (const auto &child : children) {
            Size2d childSize = child->getSize();
            width = std::max(width, childSize.width);
            totalHeight += childSize.height;
        }
        return Size2d(width + (padding.width * 2), totalHeight);
    }

    Position2d getScreenPosition() const override { return position; }
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

  private:
    void recalculatePositions();
};

#endif // GRAPHITE_LAYOUT_H