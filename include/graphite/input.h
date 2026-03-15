#ifndef GRAPHITE_INPUT_H
#define GRAPHITE_INPUT_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "graphite/text.h"
#include "opal/opal.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>

struct TextFieldChangeEvent {
    std::string text;
    std::size_t cursorIndex = 0;
    bool focused = false;
};

struct ButtonClickEvent {
    std::string label;
};

struct CheckboxToggleEvent {
    std::string label;
    bool checked = false;
};

class TextField : public UIObject {
  public:
    using ChangeCallback =
        std::function<void(const TextFieldChangeEvent &)>;

    std::string text;
    std::string placeholder;
    Font font;
    Position2d position;
    float fontSize = 0.0f;
    Size2d padding{.width = 14.0f, .height = 10.0f};
    float maximumWidth = 320.0f;
    Color textColor = Color::white();
    Color placeholderColor = Color(1.0f, 1.0f, 1.0f, 0.45f);
    Color backgroundColor = Color(0.08f, 0.09f, 0.12f, 0.94f);
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.15f);
    Color focusedBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    Color cursorColor = Color::white();

    TextField() = default;

    TextField(Font font, float maximumWidth,
              Position2d position = {.x = 0.0f, .y = 0.0f},
              std::string text = "", std::string placeholder = "");

    void initialize() override;
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    Size2d getSize() const override;

    Position2d getScreenPosition() const override { return position; }

    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    const std::string &getText() const { return text; }
    bool isFocused() const { return focused; }
    std::size_t getCursorIndex() const { return cursorIndex; }
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    TextField &setText(std::string newText);
    TextField &setPlaceholder(std::string newPlaceholder);
    TextField &setPadding(Size2d newPadding);
    TextField &setMaximumWidth(float newMaximumWidth);
    TextField &setFontSize(float newFontSize);
    TextField &setStyle(const graphite::UIStyle &newStyle);
    TextField &setOnChange(ChangeCallback callback);
    void focus();
    void blur();

  private:
    std::size_t cursorIndex = 0;
    std::size_t scrollIndex = 0;
    bool focused = false;
    graphite::BoxRendererData boxRenderer;
    Text visibleText;
    Text placeholderText;
    ChangeCallback onChange;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;

    void ensureCursorVisible(float availableWidth);
    void syncRenderers(float availableWidth);
    void processInput();
    void emitChange() const;
};

class Button : public UIObject {
  public:
    using ClickCallback =
        std::function<void(const ButtonClickEvent &)>;

    std::string label;
    Font font;
    Position2d position;
    float fontSize = 0.0f;
    Size2d padding{.width = 18.0f, .height = 12.0f};
    Size2d minimumSize{.width = 0.0f, .height = 0.0f};
    Color textColor = Color::white();
    Color backgroundColor = Color(0.15f, 0.16f, 0.2f, 0.96f);
    Color hoverBackgroundColor = Color(0.2f, 0.22f, 0.27f, 0.98f);
    Color pressedBackgroundColor = Color(1.0f, 0.55f, 0.14f, 0.96f);
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.16f);
    Color hoverBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    bool enabled = true;

    Button() = default;

    Button(Font font, std::string label,
           Position2d position = {.x = 0.0f, .y = 0.0f});

    void initialize() override;
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    Size2d getSize() const override;
    Position2d getScreenPosition() const override { return position; }
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    const std::string &getLabel() const { return label; }
    bool isHovered() const { return hovered; }
    bool isEnabled() const { return enabled; }
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Button &setLabel(std::string newLabel);
    Button &setPadding(Size2d newPadding);
    Button &setMinimumSize(Size2d newMinimumSize);
    Button &setFontSize(float newFontSize);
    Button &setStyle(const graphite::UIStyle &newStyle);
    Button &setOnClick(ClickCallback callback);
    Button &setEnabled(bool newEnabled);

  private:
    bool hovered = false;
    graphite::BoxRendererData boxRenderer;
    Text labelText;
    ClickCallback onClick;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;

    void syncLabel(const graphite::UIResolvedStyle &style);
    void emitClick() const;
};

class Checkbox : public UIObject {
  public:
    using ToggleCallback =
        std::function<void(const CheckboxToggleEvent &)>;

    std::string label;
    Font font;
    Position2d position;
    float fontSize = 0.0f;
    Size2d padding{.width = 0.0f, .height = 4.0f};
    float boxSize = 28.0f;
    float spacing = 12.0f;
    bool checked = false;
    bool enabled = true;
    Color textColor = Color::white();
    Color boxBackgroundColor = Color(0.12f, 0.13f, 0.17f, 0.96f);
    Color hoverBoxBackgroundColor = Color(0.17f, 0.18f, 0.23f, 0.98f);
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.18f);
    Color activeBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    Color checkColor = Color(1.0f, 0.55f, 0.14f, 1.0f);

    Checkbox() = default;

    Checkbox(Font font, std::string label = "", bool checked = false,
             Position2d position = {.x = 0.0f, .y = 0.0f});

    void initialize() override;
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    Size2d getSize() const override;
    Position2d getScreenPosition() const override { return position; }
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    const std::string &getLabel() const { return label; }
    bool isChecked() const { return checked; }
    bool isHovered() const { return hovered; }
    bool isEnabled() const { return enabled; }
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Checkbox &setLabel(std::string newLabel);
    Checkbox &setPadding(Size2d newPadding);
    Checkbox &setBoxSize(float newBoxSize);
    Checkbox &setSpacing(float newSpacing);
    Checkbox &setFontSize(float newFontSize);
    Checkbox &setStyle(const graphite::UIStyle &newStyle);
    Checkbox &setChecked(bool newChecked);
    Checkbox &setEnabled(bool newEnabled);
    Checkbox &setOnToggle(ToggleCallback callback);
    void toggle();

  private:
    bool hovered = false;
    graphite::BoxRendererData boxRenderer;
    Text labelText;
    ToggleCallback onToggle;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;

    void syncLabel(const graphite::UIResolvedStyle &style);
    void emitToggle() const;
};

#endif // GRAPHITE_INPUT_H
