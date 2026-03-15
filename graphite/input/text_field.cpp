#include "graphite/input.h"
#include "graphite/style.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <algorithm>

namespace {

constexpr float defaultBorderWidth = 2.0f;
constexpr float cursorWidth = 2.0f;

graphite::UIStyle makeFallbackStyle(const TextField &field) {
    graphite::UIStyle style;
    style.normal()
        .padding(field.padding)
        .foreground(field.textColor)
        .background(field.backgroundColor)
        .border(defaultBorderWidth, field.borderColor)
        .tint(field.cursorColor)
        .font(field.font);
    if (field.fontSize > 0.0f) {
        style.normal().fontSize(field.fontSize);
    }
    style.focused().border(defaultBorderWidth, field.focusedBorderColor);
    return style;
}

graphite::UIResolvedStyle resolveTextFieldStyle(const TextField &field,
                                                const graphite::UIStyle *overrideStyle,
                                                bool focused) {
    const graphite::UIStyleStateSnapshot stateSnapshot{
        .hovered = false,
        .pressed = false,
        .focused = focused,
        .disabled = false,
        .checked = false,
    };
    return graphite::resolveStyle(makeFallbackStyle(field),
                                  &graphite::Theme::current().textField,
                                  overrideStyle, stateSnapshot);
}

const Font &resolveTextFieldFont(const TextField &field,
                                 const graphite::UIStyle *overrideStyle,
                                 bool focused) {
    const graphite::UIResolvedStyle style =
        resolveTextFieldStyle(field, overrideStyle, focused);
    return style.font != nullptr ? *style.font : field.font;
}

std::size_t fitVisibleEnd(const Font &font, const std::string &text,
                          std::size_t start, float availableWidth,
                          float fontSize) {
    float width = 0.0f;
    std::size_t index = start;
    while (index < text.size()) {
        const float characterWidth =
            graphite::measureCharacterWidth(font, text[index], fontSize);
        if (index > start && width + characterWidth > availableWidth) {
            break;
        }
        width += characterWidth;
        ++index;
        if (width > availableWidth) {
            break;
        }
    }
    return index;
}

} // namespace

TextField::TextField(Font font, float maximumWidth, Position2d position,
                     std::string text, std::string placeholder)
    : text(graphite::sanitizeText(font, text)),
      placeholder(std::move(placeholder)), font(std::move(font)),
      position(position), maximumWidth(std::max(maximumWidth, 0.0f)) {
    cursorIndex = this->text.size();
}

Size2d TextField::getSize() const {
    const graphite::UIResolvedStyle style = resolveTextFieldStyle(
        *this, usesLocalStyle ? &localStyle : nullptr, focused);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    const float width = std::max(maximumWidth, style.padding.width * 2.0f);
    const float height = std::max(
        graphite::getLineHeight(resolvedFont, style.fontSize) +
            (style.padding.height * 2.0f),
        style.padding.height * 2.0f);
    return {.width = width, .height = height};
}

TextField &TextField::setText(std::string newText) {
    const std::string sanitized = graphite::sanitizeText(
        resolveTextFieldFont(*this, usesLocalStyle ? &localStyle : nullptr,
                             focused),
        newText);
    if (sanitized == text) {
        return *this;
    }
    text = sanitized;
    cursorIndex = text.size();
    scrollIndex = std::min(scrollIndex, cursorIndex);
    emitChange();
    return *this;
}

TextField &TextField::setPlaceholder(std::string newPlaceholder) {
    placeholder = std::move(newPlaceholder);
    return *this;
}

TextField &TextField::setPadding(Size2d newPadding) {
    padding = {.width = std::max(newPadding.width, 0.0f),
               .height = std::max(newPadding.height, 0.0f)};
    return *this;
}

TextField &TextField::setMaximumWidth(float newMaximumWidth) {
    maximumWidth = std::max(newMaximumWidth, 0.0f);
    return *this;
}

TextField &TextField::setFontSize(float newFontSize) {
    fontSize = std::max(newFontSize, 0.0f);
    return *this;
}

TextField &TextField::setStyle(const graphite::UIStyle &newStyle) {
    localStyle = newStyle;
    usesLocalStyle = true;
    return *this;
}

TextField &TextField::setOnChange(ChangeCallback callback) {
    onChange = std::move(callback);
    return *this;
}

void TextField::focus() {
    focused = true;
    if (Window::mainWindow != nullptr &&
        !Window::mainWindow->isTextInputActive()) {
        Window::mainWindow->startTextInput();
    }
}

void TextField::blur() {
    focused = false;
    if (Window::mainWindow != nullptr &&
        Window::mainWindow->isTextInputActive()) {
        Window::mainWindow->stopTextInput();
    }
}

void TextField::emitChange() const {
    if (onChange) {
        onChange({.text = text, .cursorIndex = cursorIndex, .focused = focused});
    }
}

void TextField::ensureCursorVisible(float availableWidth) {
    const graphite::UIResolvedStyle style = resolveTextFieldStyle(
        *this, usesLocalStyle ? &localStyle : nullptr, focused);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;

    cursorIndex = std::min(cursorIndex, text.size());
    scrollIndex = std::min(scrollIndex, cursorIndex);

    while (scrollIndex < cursorIndex &&
           graphite::measureTextWidth(resolvedFont, text, scrollIndex,
                                      cursorIndex, style.fontSize) >
               availableWidth) {
        ++scrollIndex;
    }

    while (scrollIndex > 0 &&
           graphite::measureTextWidth(resolvedFont, text, scrollIndex - 1,
                                      cursorIndex, style.fontSize) <=
               availableWidth) {
        --scrollIndex;
    }
}

void TextField::syncRenderers(float availableWidth) {
    const graphite::UIResolvedStyle style = resolveTextFieldStyle(
        *this, usesLocalStyle ? &localStyle : nullptr, focused);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    ensureCursorVisible(availableWidth);

    const std::size_t visibleEnd = fitVisibleEnd(
        resolvedFont, text, scrollIndex, availableWidth, style.fontSize);
    const std::string visibleValue =
        text.substr(scrollIndex, visibleEnd - scrollIndex);

    visibleText.font = resolvedFont;
    visibleText.fontSize = style.fontSize;
    visibleText.color = style.foregroundColor;
    visibleText.content = visibleValue;

    placeholderText.font = resolvedFont;
    placeholderText.fontSize = style.fontSize;
    placeholderText.color =
        Color(style.foregroundColor.r, style.foregroundColor.g,
              style.foregroundColor.b, placeholderColor.a);
    placeholderText.content = placeholder;

    const float lineHeight =
        graphite::getLineHeight(resolvedFont, style.fontSize);
    const float contentLeft = position.x + style.padding.width;
    const float contentTop =
        position.y + ((getSize().height - lineHeight) * 0.5f);

    visibleText.setScreenPosition({.x = contentLeft, .y = contentTop});
    placeholderText.setScreenPosition({.x = contentLeft, .y = contentTop});
}

void TextField::processInput() {
    Window *window = Window::mainWindow;
    if (window == nullptr) {
        return;
    }

    const graphite::UIResolvedStyle style = resolveTextFieldStyle(
        *this, usesLocalStyle ? &localStyle : nullptr, focused);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    const Size2d fieldSize = getSize();
    const auto [cursorX, cursorY] = window->getCursorPosition();
    const bool containsCursor =
        static_cast<float>(cursorX) >= position.x &&
        static_cast<float>(cursorX) <= position.x + fieldSize.width &&
        static_cast<float>(cursorY) >= position.y &&
        static_cast<float>(cursorY) <= position.y + fieldSize.height;

    if (window->isMouseButtonPressed(MouseButton::Left)) {
        if (containsCursor) {
            focus();
            const float contentLeft = position.x + style.padding.width;
            const float localX =
                std::max(0.0f, static_cast<float>(cursorX) - contentLeft);
            std::size_t nextCursor = scrollIndex;
            float accumulatedWidth = 0.0f;
            while (nextCursor < text.size()) {
                const float characterWidth = graphite::measureCharacterWidth(
                    resolvedFont, text[nextCursor], style.fontSize);
                if (localX < accumulatedWidth + (characterWidth * 0.5f)) {
                    break;
                }
                accumulatedWidth += characterWidth;
                ++nextCursor;
            }
            cursorIndex = nextCursor;
        } else if (focused) {
            blur();
        }
    }

    if (!focused) {
        return;
    }

    bool changed = false;
    const float availableWidth =
        std::max(0.0f, fieldSize.width - (style.padding.width * 2.0f));

    if (window->isKeyPressed(Key::Escape)) {
        blur();
        return;
    }

    if (window->isKeyPressed(Key::Left) && cursorIndex > 0) {
        --cursorIndex;
    }
    if (window->isKeyPressed(Key::Right) && cursorIndex < text.size()) {
        ++cursorIndex;
    }
    if (window->isKeyPressed(Key::Home)) {
        cursorIndex = 0;
    }
    if (window->isKeyPressed(Key::End)) {
        cursorIndex = text.size();
    }
    if (window->isKeyPressed(Key::Backspace) && cursorIndex > 0) {
        text.erase(cursorIndex - 1, 1);
        --cursorIndex;
        changed = true;
    }
    if (window->isKeyPressed(Key::Delete) && cursorIndex < text.size()) {
        text.erase(cursorIndex, 1);
        changed = true;
    }

    const std::string typed =
        graphite::sanitizeText(resolvedFont, window->getTextInput());
    if (!typed.empty()) {
        text.insert(cursorIndex, typed);
        cursorIndex += typed.size();
        changed = true;
    }

    ensureCursorVisible(availableWidth);

    if (changed) {
        emitChange();
    }
}

void TextField::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
}

void TextField::render(float dt,
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
        atlas_error("TextField::render requires a valid command buffer");
        return;
    }

    processInput();

    const graphite::UIResolvedStyle style = resolveTextFieldStyle(
        *this, usesLocalStyle ? &localStyle : nullptr, focused);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    const Size2d fieldSize = getSize();
    const float availableWidth =
        std::max(0.0f, fieldSize.width - (style.padding.width * 2.0f));
    syncRenderers(availableWidth);

    graphite::renderStyledBox(boxRenderer, id, commandBuffer, position,
                              fieldSize, style);

    if (!text.empty()) {
        visibleText.render(dt, commandBuffer, updatePipeline);
    } else if (!placeholder.empty()) {
        placeholderText.render(dt, commandBuffer, updatePipeline);
    }

    if (focused) {
        const float lineHeight =
            graphite::getLineHeight(resolvedFont, style.fontSize);
        const float cursorHeight = std::max(0.0f, lineHeight);
        const float cursorOffset = graphite::measureTextWidth(
            resolvedFont, text, scrollIndex, cursorIndex, style.fontSize);
        graphite::UIResolvedStyle cursorStyle;
        cursorStyle.backgroundColor = style.tintColor;
        cursorStyle.cornerRadius = std::min(cursorWidth, cursorHeight) * 0.5f;
        graphite::renderStyledBox(
            boxRenderer, id, commandBuffer,
            {.x = position.x + style.padding.width + cursorOffset,
             .y = position.y + ((fieldSize.height - cursorHeight) * 0.5f)},
            {.width = cursorWidth, .height = cursorHeight}, cursorStyle);
    }
}
