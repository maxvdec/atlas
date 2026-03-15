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

graphite::UIStyle makeFallbackStyle(const Checkbox &checkbox) {
    graphite::UIStyle style;
    style.normal()
        .padding(checkbox.padding)
        .foreground(checkbox.textColor)
        .background(checkbox.boxBackgroundColor)
        .border(defaultBorderWidth, checkbox.borderColor)
        .tint(checkbox.checkColor)
        .font(checkbox.font);
    if (checkbox.fontSize > 0.0f) {
        style.normal().fontSize(checkbox.fontSize);
    }
    style.hovered()
        .background(checkbox.hoverBoxBackgroundColor)
        .border(defaultBorderWidth, checkbox.activeBorderColor);
    style.checked()
        .border(defaultBorderWidth, checkbox.activeBorderColor)
        .tint(checkbox.checkColor);
    style.disabled()
        .foreground(Color(checkbox.textColor.r, checkbox.textColor.g,
                          checkbox.textColor.b, checkbox.textColor.a * 0.55f))
        .background(Color(checkbox.boxBackgroundColor.r,
                          checkbox.boxBackgroundColor.g,
                          checkbox.boxBackgroundColor.b,
                          checkbox.boxBackgroundColor.a * 0.55f))
        .border(defaultBorderWidth,
                Color(checkbox.borderColor.r, checkbox.borderColor.g,
                      checkbox.borderColor.b, checkbox.borderColor.a * 0.55f))
        .tint(Color(checkbox.checkColor.r, checkbox.checkColor.g,
                    checkbox.checkColor.b, checkbox.checkColor.a * 0.55f));
    return style;
}

} // namespace

Checkbox::Checkbox(Font font, std::string label, bool checked,
                   Position2d position)
    : label(sanitizeLabel(font, label)), font(std::move(font)),
      position(position), checked(checked) {}

Size2d Checkbox::getSize() const {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().checkbox,
        usesLocalStyle ? &localStyle : nullptr);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    const float labelWidth =
        graphite::measureTextWidth(resolvedFont, label, style.fontSize);
    const float contentWidth =
        boxSize + (label.empty() ? 0.0f : spacing + labelWidth);
    const float contentHeight =
        std::max(boxSize, graphite::getLineHeight(resolvedFont, style.fontSize));
    return {.width = contentWidth + (style.padding.width * 2.0f),
            .height = contentHeight + (style.padding.height * 2.0f)};
}

Checkbox &Checkbox::setLabel(std::string newLabel) {
    label = sanitizeLabel(font, newLabel);
    return *this;
}

Checkbox &Checkbox::setPadding(Size2d newPadding) {
    padding = {.width = std::max(newPadding.width, 0.0f),
               .height = std::max(newPadding.height, 0.0f)};
    return *this;
}

Checkbox &Checkbox::setBoxSize(float newBoxSize) {
    boxSize = std::max(newBoxSize, 0.0f);
    return *this;
}

Checkbox &Checkbox::setSpacing(float newSpacing) {
    spacing = std::max(newSpacing, 0.0f);
    return *this;
}

Checkbox &Checkbox::setFontSize(float newFontSize) {
    fontSize = std::max(newFontSize, 0.0f);
    return *this;
}

Checkbox &Checkbox::setStyle(const graphite::UIStyle &newStyle) {
    localStyle = newStyle;
    usesLocalStyle = true;
    return *this;
}

Checkbox &Checkbox::setChecked(bool newChecked) {
    if (checked == newChecked) {
        return *this;
    }
    checked = newChecked;
    emitToggle();
    return *this;
}

Checkbox &Checkbox::setEnabled(bool newEnabled) {
    enabled = newEnabled;
    return *this;
}

Checkbox &Checkbox::setOnToggle(ToggleCallback callback) {
    onToggle = std::move(callback);
    return *this;
}

void Checkbox::toggle() { setChecked(!checked); }

void Checkbox::emitToggle() const {
    if (onToggle) {
        onToggle({.label = label, .checked = checked});
    }
}

void Checkbox::syncLabel(const graphite::UIResolvedStyle &style) {
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
    const float contentLeft = position.x + style.padding.width + boxSize +
                              (label.empty() ? 0.0f : spacing);

    labelText.setScreenPosition({.x = contentLeft, .y = contentTop});
}

void Checkbox::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
}

void Checkbox::render(float dt,
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
        atlas_error("Checkbox::render requires a valid command buffer");
        return;
    }

    const Size2d size = getSize();
    hovered = false;

    Window *window = Window::mainWindow;
    if (window != nullptr) {
        const auto [cursorX, cursorY] = window->getCursorPosition();
        const bool containsCursor =
            static_cast<float>(cursorX) >= position.x &&
            static_cast<float>(cursorX) <= position.x + size.width &&
            static_cast<float>(cursorY) >= position.y &&
            static_cast<float>(cursorY) <= position.y + size.height;
        hovered = enabled && containsCursor;
        if (hovered && window->isMouseButtonPressed(MouseButton::Left)) {
            toggle();
        }
    }

    const graphite::UIStyleStateSnapshot stateSnapshot{
        .hovered = hovered,
        .pressed = false,
        .focused = false,
        .disabled = !enabled,
        .checked = checked,
    };
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().checkbox,
        usesLocalStyle ? &localStyle : nullptr, stateSnapshot);

    const float boxLeft = position.x + style.padding.width;
    const float boxTop = position.y + ((size.height - boxSize) * 0.5f);
    graphite::renderStyledBox(boxRenderer, id, commandBuffer,
                              {.x = boxLeft, .y = boxTop},
                              {.width = boxSize, .height = boxSize}, style);

    if (checked) {
        graphite::UIResolvedStyle activeStyle;
        activeStyle.backgroundColor = style.tintColor;
        activeStyle.tintColor = style.tintColor;
        activeStyle.cornerRadius =
            std::max(0.0f, style.cornerRadius - std::max(boxSize * 0.18f, 4.0f));
        const float inset = std::max(5.0f, boxSize * 0.22f);
        graphite::renderStyledBox(
            boxRenderer, id, commandBuffer,
            {.x = boxLeft + inset, .y = boxTop + inset},
            {.width = std::max(0.0f, boxSize - (inset * 2.0f)),
             .height = std::max(0.0f, boxSize - (inset * 2.0f))},
            activeStyle);
    }

    syncLabel(style);
    if (!label.empty()) {
        labelText.render(dt, commandBuffer, updatePipeline);
    }
}
