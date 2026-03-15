#include "graphite/input.h"
#include "graphite/style.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <algorithm>

namespace {

constexpr float defaultBorderWidth = 2.0f;

std::string sanitizeLabel(const Font &font, const std::string &input) {
    return graphite::sanitizeText(font, input);
}

graphite::UIStyle makeFallbackStyle(const Button &button) {
    graphite::UIStyle style;
    style.normal()
        .padding(button.padding)
        .foreground(button.textColor)
        .background(button.backgroundColor)
        .border(defaultBorderWidth, button.borderColor)
        .font(button.font);
    if (button.fontSize > 0.0f) {
        style.normal().fontSize(button.fontSize);
    }
    style.hovered()
        .background(button.hoverBackgroundColor)
        .border(defaultBorderWidth, button.hoverBorderColor);
    style.pressed()
        .background(button.pressedBackgroundColor)
        .border(defaultBorderWidth, button.hoverBorderColor);
    style.disabled()
        .foreground(Color(button.textColor.r, button.textColor.g, button.textColor.b,
                          button.textColor.a * 0.55f))
        .background(Color(button.backgroundColor.r, button.backgroundColor.g,
                          button.backgroundColor.b, button.backgroundColor.a * 0.55f))
        .border(defaultBorderWidth,
                Color(button.borderColor.r, button.borderColor.g,
                      button.borderColor.b, button.borderColor.a * 0.55f));
    return style;
}

} // namespace

Button::Button(Font font, std::string label, Position2d position)
    : label(sanitizeLabel(font, label)), font(std::move(font)),
      position(position) {}

Size2d Button::getSize() const {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().button,
        usesLocalStyle ? &localStyle : nullptr);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    const float width = std::max(
        minimumSize.width,
        graphite::measureTextWidth(resolvedFont, label, style.fontSize) +
            (style.padding.width * 2.0f));
    const float height = std::max(
        minimumSize.height,
        graphite::getLineHeight(resolvedFont, style.fontSize) +
            (style.padding.height * 2.0f));
    return {.width = width, .height = height};
}

Button &Button::setLabel(std::string newLabel) {
    label = sanitizeLabel(font, newLabel);
    return *this;
}

Button &Button::setPadding(Size2d newPadding) {
    padding = {.width = std::max(newPadding.width, 0.0f),
               .height = std::max(newPadding.height, 0.0f)};
    return *this;
}

Button &Button::setMinimumSize(Size2d newMinimumSize) {
    minimumSize = {.width = std::max(newMinimumSize.width, 0.0f),
                   .height = std::max(newMinimumSize.height, 0.0f)};
    return *this;
}

Button &Button::setFontSize(float newFontSize) {
    fontSize = std::max(newFontSize, 0.0f);
    return *this;
}

Button &Button::setStyle(const graphite::UIStyle &newStyle) {
    localStyle = newStyle;
    usesLocalStyle = true;
    return *this;
}

Button &Button::setOnClick(ClickCallback callback) {
    onClick = std::move(callback);
    return *this;
}

Button &Button::setEnabled(bool newEnabled) {
    enabled = newEnabled;
    return *this;
}

void Button::emitClick() const {
    if (onClick) {
        onClick({.label = label});
    }
}

void Button::syncLabel(const graphite::UIResolvedStyle &style) {
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    labelText.font = resolvedFont;
    labelText.fontSize = style.fontSize;
    labelText.color = style.foregroundColor;
    labelText.content = label;

    const Size2d size = getSize();
    const float contentTop =
        position.y +
        ((size.height -
          graphite::getLineHeight(resolvedFont, style.fontSize)) *
         0.5f);
    const float contentLeft =
        position.x +
        ((size.width -
          graphite::measureTextWidth(resolvedFont, label, style.fontSize)) *
         0.5f);

    labelText.setScreenPosition({.x = contentLeft, .y = contentTop});
}

void Button::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
}

void Button::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    (void)updatePipeline;

    if (boxRenderer.shader.shader == nullptr || boxRenderer.vao == nullptr ||
        boxRenderer.vertexBuffer == nullptr) {
        initialize();
    }

    for (auto &component : components) {
        component->update(dt);
    }

    if (commandBuffer == nullptr) {
        atlas_error("Button::render requires a valid command buffer");
        return;
    }

    Window *window = Window::mainWindow;
    const Size2d size = getSize();
    bool pressed = false;
    hovered = false;

    if (window != nullptr) {
        const auto [cursorX, cursorY] = window->getCursorPosition();
        const bool containsCursor =
            static_cast<float>(cursorX) >= position.x &&
            static_cast<float>(cursorX) <= position.x + size.width &&
            static_cast<float>(cursorY) >= position.y &&
            static_cast<float>(cursorY) <= position.y + size.height;
        hovered = enabled && containsCursor;
        pressed = hovered && window->isMouseButtonPressed(MouseButton::Left);
        if (pressed) {
            emitClick();
        }
    }

    const graphite::UIStyleStateSnapshot stateSnapshot{
        .hovered = hovered,
        .pressed = pressed,
        .focused = false,
        .disabled = !enabled,
        .checked = false,
    };
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().button,
        usesLocalStyle ? &localStyle : nullptr, stateSnapshot);

    graphite::renderStyledBox(boxRenderer, id, commandBuffer, position, size,
                              style);

    syncLabel(style);
    if (!label.empty()) {
        labelText.render(dt, commandBuffer, updatePipeline);
    }
}
