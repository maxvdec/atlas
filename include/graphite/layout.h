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
#include "graphite/style.h"
#include <atlas/component.h>
#include <algorithm>
#include <vector>

enum class ElementAlignment {
    Top,
    Center,
    Bottom,
};

enum class LayoutAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
};

namespace graphite {
Position2d getAnchoredTopLeft(Position2d position, Size2d size,
                              LayoutAnchor anchor);
} // namespace graphite

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
    ElementAlignment alignment = ElementAlignment::Top;
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    void addChild(UIObject *child);
    void setChildren(const std::vector<UIObject *> &newChildren);

    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().column,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float width = 0.0f;
        float totalHeight = effectivePadding.height * 2.0f;
        bool hasChild = false;
        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            width = std::max(width, childSize.width);
            totalHeight += childSize.height;
            if (hasChild) {
                totalHeight += spacing;
            }
            hasChild = true;
        }
        return Size2d{
            .width =
                std::max(width + (effectivePadding.width * 2.0f), maxSize.width),
            .height = std::max(totalHeight, maxSize.height),
        };
    }

    Position2d getScreenPosition() const override { return position; }
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Column &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

class Row : public UIObject {
  public:
    Row(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }

    Row(const std::vector<UIObject *> &children, float spacing = 0.0f,
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
    ElementAlignment alignment = ElementAlignment::Center;
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    void addChild(UIObject *child);
    void setChildren(const std::vector<UIObject *> &newChildren);

    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().row,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float totalWidth = effectivePadding.width * 2.0f;
        float maxHeight = 0.0f;
        bool hasChild = false;

        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            if (hasChild) {
                totalWidth += spacing;
            }
            totalWidth += childSize.width;
            maxHeight = std::max(maxHeight, childSize.height);
            hasChild = true;
        }

        return Size2d{
            .width = std::max(totalWidth, maxSize.width),
            .height = std::max(maxHeight + (effectivePadding.height * 2.0f),
                               maxSize.height),
        };
    }

    Position2d getScreenPosition() const override { return position; }

    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Row &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

class Stack : public UIObject {
  public:
    Stack(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }

    Stack(const std::vector<UIObject *> &children,
          Size2d padding = {.width = 0.0f, .height = 0.0f},
          Position2d pos = {.x = 0, .y = 0})
        : padding(padding), children(children), position(pos) {
        recalculatePositions();
    }

    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override;

    Size2d maxSize{.width = 0.0f, .height = 0.0f};
    Size2d padding{.width = 0.0f, .height = 0.0f};
    std::vector<UIObject *> children;
    Position2d position;
    ElementAlignment horizontalAlignment = ElementAlignment::Top;
    ElementAlignment verticalAlignment = ElementAlignment::Top;
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    void addChild(UIObject *child);
    void setChildren(const std::vector<UIObject *> &newChildren);

    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().stack,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float width = 0.0f;
        float height = 0.0f;

        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            width = std::max(width, childSize.width);
            height = std::max(height, childSize.height);
        }

        return Size2d{
            .width =
                std::max(width + (effectivePadding.width * 2.0f), maxSize.width),
            .height = std::max(height + (effectivePadding.height * 2.0f),
                               maxSize.height),
        };
    }

    Position2d getScreenPosition() const override { return position; }

    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Stack &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

#endif // GRAPHITE_LAYOUT_H
