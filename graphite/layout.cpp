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

namespace {

LayoutAnchor getLayoutAnchorForChild(const UIObject *child) {
    if (const auto *column = dynamic_cast<const Column *>(child)) {
        return column->anchor;
    }
    if (const auto *row = dynamic_cast<const Row *>(child)) {
        return row->anchor;
    }
    return LayoutAnchor::TopLeft;
}

Position2d getPositionForTopLeft(Position2d topLeft, Size2d size,
                                 LayoutAnchor anchor) {
    Position2d position = topLeft;

    switch (anchor) {
    case LayoutAnchor::TopLeft:
        break;
    case LayoutAnchor::TopCenter:
        position.x += size.width / 2.0f;
        break;
    case LayoutAnchor::TopRight:
        position.x += size.width;
        break;
    case LayoutAnchor::CenterLeft:
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::Center:
        position.x += size.width / 2.0f;
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::CenterRight:
        position.x += size.width;
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::BottomLeft:
        position.y += size.height;
        break;
    case LayoutAnchor::BottomCenter:
        position.x += size.width / 2.0f;
        position.y += size.height;
        break;
    case LayoutAnchor::BottomRight:
        position.x += size.width;
        position.y += size.height;
        break;
    }

    return position;
}

void setChildTopLeft(UIObject *child, const Position2d &topLeft,
                     const Size2d &childSize) {
    if (child == nullptr) {
        return;
    }

    child->setScreenPosition(getPositionForTopLeft(
        topLeft, childSize, getLayoutAnchorForChild(child)));
}

} // namespace

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
        if (child == nullptr) {
            continue;
        }
        child->setViewMatrix(view);
    }
}

void Column::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setProjectionMatrix(projection);
    }
}

void Column::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    recalculatePositions();
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Column::recalculatePositions() {
    Size2d layoutSize = getSize();
    Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);

    float currentY = topLeft.y + padding.height;

    float contentWidth = layoutSize.width - (padding.width * 2.0f);

    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        Size2d childSize = child->getSize();
        float childX = topLeft.x + padding.width;

        switch (alignment) {
        case ElementAlignment::Top:
            childX = topLeft.x + padding.width;
            break;

        case ElementAlignment::Center:
            childX = topLeft.x + padding.width +
                     ((contentWidth - childSize.width) / 2.0f);
            break;

        case ElementAlignment::Bottom:
            childX =
                topLeft.x + layoutSize.width - padding.width - childSize.width;
            break;
        }

        setChildTopLeft(child, {.x = childX, .y = currentY}, childSize);

        currentY += childSize.height + spacing;
    }
}

void Row::addChild(UIObject *child) {
    children.push_back(child);
    recalculatePositions();
}

void Row::setChildren(const std::vector<UIObject *> &newChildren) {
    children = newChildren;
    recalculatePositions();
}

void Row::setViewMatrix(const glm::mat4 &view) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setViewMatrix(view);
    }
}

void Row::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setProjectionMatrix(projection);
    }
}

void Row::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                 bool updatePipeline) {
    recalculatePositions();
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Row::recalculatePositions() {
    Size2d layoutSize = getSize();
    Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);

    float currentX = topLeft.x + padding.width;

    float contentHeight = layoutSize.height - (padding.height * 2.0f);

    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        Size2d childSize = child->getSize();
        float childY = topLeft.y + padding.height;

        switch (alignment) {
        case ElementAlignment::Top:
            childY = topLeft.y + padding.height;
            break;

        case ElementAlignment::Center:
            childY = topLeft.y + padding.height +
                     ((contentHeight - childSize.height) / 2.0f);
            break;

        case ElementAlignment::Bottom:
            childY = topLeft.y + layoutSize.height - padding.height -
                     childSize.height;
            break;
        }

        setChildTopLeft(child, {.x = currentX, .y = childY}, childSize);

        currentX += childSize.width + spacing;
    }
}

Position2d graphite::getAnchoredTopLeft(Position2d position, Size2d size,
                                        LayoutAnchor anchor) {
    Position2d topLeft = position;

    switch (anchor) {
    case LayoutAnchor::TopLeft:
        break;
    case LayoutAnchor::TopCenter:
        topLeft.x -= size.width / 2.0f;
        break;
    case LayoutAnchor::TopRight:
        topLeft.x -= size.width;
        break;
    case LayoutAnchor::CenterLeft:
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::Center:
        topLeft.x -= size.width / 2.0f;
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::CenterRight:
        topLeft.x -= size.width;
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::BottomLeft:
        topLeft.y -= size.height;
        break;
    case LayoutAnchor::BottomCenter:
        topLeft.x -= size.width / 2.0f;
        topLeft.y -= size.height;
        break;
    case LayoutAnchor::BottomRight:
        topLeft.x -= size.width;
        topLeft.y -= size.height;
        break;
    }

    return topLeft;
}
