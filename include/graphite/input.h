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

/**
 * @brief Event payload emitted when a text field changes or focus changes.
 */
struct TextFieldChangeEvent {
    /** @brief Current text content after the change. */
    std::string text;
    /** @brief Cursor position within the current text. */
    std::size_t cursorIndex = 0;
    /** @brief Whether the text field is currently focused. */
    bool focused = false;
};

/**
 * @brief Event payload emitted when a button is clicked.
 */
struct ButtonClickEvent {
    /** @brief Label of the button that was activated. */
    std::string label;
};

/**
 * @brief Event payload emitted when a checkbox changes state.
 */
struct CheckboxToggleEvent {
    /** @brief Label of the checkbox that toggled. */
    std::string label;
    /** @brief New checked state after the toggle. */
    bool checked = false;
};

/**
 * @brief Single-line editable text input for Graphite interfaces.
 *
 * \subsection graphite-textfield-example Example
 * ```cpp
 * TextField nameField(Font::getFont("Inter"), 280.0f, {.x = 24.0f, .y = 24.0f});
 * nameField.setPlaceholder("Project name");
 * nameField.setOnChange([](const TextFieldChangeEvent &event) {
 *     if (event.focused) {
 *         // React to input while the field is active.
 *     }
 * });
 * ```
 */
class TextField : public UIObject {
  public:
    /** @brief Callback invoked when text or focus state changes. */
    using ChangeCallback =
        std::function<void(const TextFieldChangeEvent &)>;

    /** @brief Current text value. */
    std::string text;
    /** @brief Text shown while the field is empty. */
    std::string placeholder;
    /** @brief Font used to render field contents. */
    Font font;
    /** @brief Screen-space top-left position of the field. */
    Position2d position;
    /** @brief Explicit font size override. Uses the font's native size when 0. */
    float fontSize = 0.0f;
    /** @brief Inner padding applied around the text content. */
    Size2d padding{.width = 14.0f, .height = 10.0f};
    /** @brief Maximum width available before horizontal scrolling begins. */
    float maximumWidth = 320.0f;
    /** @brief Color used for committed text. */
    Color textColor = Color::white();
    /** @brief Color used for placeholder text. */
    Color placeholderColor = Color(1.0f, 1.0f, 1.0f, 0.45f);
    /** @brief Background fill for the field. */
    Color backgroundColor = Color(0.08f, 0.09f, 0.12f, 0.94f);
    /** @brief Border color used while unfocused. */
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.15f);
    /** @brief Border color used while focused. */
    Color focusedBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    /** @brief Color of the insertion cursor. */
    Color cursorColor = Color::white();

    /** @brief Constructs an empty text field. */
    TextField() = default;

    /**
     * @brief Constructs a text field with font, width, position, text, and
     * placeholder.
     */
    TextField(Font font, float maximumWidth,
              Position2d position = {.x = 0.0f, .y = 0.0f},
              std::string text = "", std::string placeholder = "");

    /** @brief Prepares the text field renderers and layout state. */
    void initialize() override;
    /** @brief Draws the field, text, and caret and processes input. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    /** @brief Returns the current rendered size of the field. */
    Size2d getSize() const override;

    /** @brief Returns the field screen position. */
    Position2d getScreenPosition() const override { return position; }

    /** @brief Sets the field screen position. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    /** @brief Returns the current text value. */
    const std::string &getText() const { return text; }
    /** @brief Returns whether the field is currently focused. */
    bool isFocused() const { return focused; }
    /** @brief Returns the current cursor index. */
    std::size_t getCursorIndex() const { return cursorIndex; }
    /** @brief Returns a mutable local style override for this field. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the text content. */
    TextField &setText(std::string newText);
    /** @brief Replaces the placeholder string. */
    TextField &setPlaceholder(std::string newPlaceholder);
    /** @brief Replaces the inner padding. */
    TextField &setPadding(Size2d newPadding);
    /** @brief Replaces the maximum width before scrolling. */
    TextField &setMaximumWidth(float newMaximumWidth);
    /** @brief Replaces the explicit font size override. */
    TextField &setFontSize(float newFontSize);
    /** @brief Replaces the local style override. */
    TextField &setStyle(const graphite::UIStyle &newStyle);
    /** @brief Registers the callback invoked when text or focus changes. */
    TextField &setOnChange(ChangeCallback callback);
    /** @brief Gives keyboard focus to the field. */
    void focus();
    /** @brief Removes keyboard focus from the field. */
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

/**
 * @brief Clickable push button for Graphite interfaces.
 *
 * \subsection graphite-button-example Example
 * ```cpp
 * Button saveButton(Font::getFont("Inter"), "Save", {.x = 24.0f, .y = 72.0f});
 * saveButton.setOnClick([](const ButtonClickEvent &event) {
 *     std::cout << "Clicked: " << event.label << '\n';
 * });
 * saveButton.style().hovered().background(Color(0.22f, 0.24f, 0.3f, 1.0f));
 * ```
 */
class Button : public UIObject {
  public:
    /** @brief Callback invoked when the button is clicked. */
    using ClickCallback =
        std::function<void(const ButtonClickEvent &)>;

    /** @brief Text shown inside the button. */
    std::string label;
    /** @brief Font used to render the label. */
    Font font;
    /** @brief Screen-space top-left position of the button. */
    Position2d position;
    /** @brief Explicit font size override. Uses the font's native size when 0. */
    float fontSize = 0.0f;
    /** @brief Inner padding applied around the label. */
    Size2d padding{.width = 18.0f, .height = 12.0f};
    /** @brief Minimum size enforced after measuring the label. */
    Size2d minimumSize{.width = 0.0f, .height = 0.0f};
    /** @brief Label color. */
    Color textColor = Color::white();
    /** @brief Default button background. */
    Color backgroundColor = Color(0.15f, 0.16f, 0.2f, 0.96f);
    /** @brief Background used while the cursor hovers the button. */
    Color hoverBackgroundColor = Color(0.2f, 0.22f, 0.27f, 0.98f);
    /** @brief Background used while the button is pressed. */
    Color pressedBackgroundColor = Color(1.0f, 0.55f, 0.14f, 0.96f);
    /** @brief Border color used while idle. */
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.16f);
    /** @brief Border color used while hovered. */
    Color hoverBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    /** @brief Whether the button can currently be interacted with. */
    bool enabled = true;

    /** @brief Constructs an empty button. */
    Button() = default;

    /** @brief Constructs a button with font, label, and position. */
    Button(Font font, std::string label,
           Position2d position = {.x = 0.0f, .y = 0.0f});

    /** @brief Prepares the button renderers and label state. */
    void initialize() override;
    /** @brief Draws the button and dispatches click interaction. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    /** @brief Returns the current rendered size of the button. */
    Size2d getSize() const override;
    /** @brief Returns the button screen position. */
    Position2d getScreenPosition() const override { return position; }
    /** @brief Sets the button screen position. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    /** @brief Returns the current label. */
    const std::string &getLabel() const { return label; }
    /** @brief Returns whether the cursor is currently hovering the button. */
    bool isHovered() const { return hovered; }
    /** @brief Returns whether the button is enabled. */
    bool isEnabled() const { return enabled; }
    /** @brief Returns a mutable local style override for this button. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the label text. */
    Button &setLabel(std::string newLabel);
    /** @brief Replaces the inner padding. */
    Button &setPadding(Size2d newPadding);
    /** @brief Replaces the minimum button size. */
    Button &setMinimumSize(Size2d newMinimumSize);
    /** @brief Replaces the explicit font size override. */
    Button &setFontSize(float newFontSize);
    /** @brief Replaces the local style override. */
    Button &setStyle(const graphite::UIStyle &newStyle);
    /** @brief Registers the callback invoked when the button is clicked. */
    Button &setOnClick(ClickCallback callback);
    /** @brief Enables or disables interaction. */
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

/**
 * @brief Toggleable checkbox with an optional text label.
 *
 * \subsection graphite-checkbox-example Example
 * ```cpp
 * Checkbox shadows(Font::getFont("Inter"), "Shadows", true,
 *                  {.x = 24.0f, .y = 120.0f});
 * shadows.setOnToggle([](const CheckboxToggleEvent &event) {
 *     // Use event.checked to update application state.
 * });
 * ```
 */
class Checkbox : public UIObject {
  public:
    /** @brief Callback invoked when the checked state changes. */
    using ToggleCallback =
        std::function<void(const CheckboxToggleEvent &)>;

    /** @brief Label displayed next to the checkbox. */
    std::string label;
    /** @brief Font used to render the label. */
    Font font;
    /** @brief Screen-space top-left position of the checkbox. */
    Position2d position;
    /** @brief Explicit font size override. Uses the font's native size when 0. */
    float fontSize = 0.0f;
    /** @brief Extra padding around the checkbox row. */
    Size2d padding{.width = 0.0f, .height = 4.0f};
    /** @brief Size of the square check box. */
    float boxSize = 28.0f;
    /** @brief Gap between the box and the label. */
    float spacing = 12.0f;
    /** @brief Current checked state. */
    bool checked = false;
    /** @brief Whether the checkbox can currently be interacted with. */
    bool enabled = true;
    /** @brief Label color. */
    Color textColor = Color::white();
    /** @brief Default fill color for the box. */
    Color boxBackgroundColor = Color(0.12f, 0.13f, 0.17f, 0.96f);
    /** @brief Fill color used while hovered. */
    Color hoverBoxBackgroundColor = Color(0.17f, 0.18f, 0.23f, 0.98f);
    /** @brief Default border color. */
    Color borderColor = Color(1.0f, 1.0f, 1.0f, 0.18f);
    /** @brief Border color used while checked or hovered in the active state. */
    Color activeBorderColor = Color(1.0f, 0.55f, 0.14f, 1.0f);
    /** @brief Color used for the check mark. */
    Color checkColor = Color(1.0f, 0.55f, 0.14f, 1.0f);

    /** @brief Constructs an empty checkbox. */
    Checkbox() = default;

    /** @brief Constructs a checkbox with font, label, state, and position. */
    Checkbox(Font font, std::string label = "", bool checked = false,
             Position2d position = {.x = 0.0f, .y = 0.0f});

    /** @brief Prepares the checkbox renderers and label state. */
    void initialize() override;
    /** @brief Draws the checkbox and dispatches toggle interaction. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    /** @brief Returns the current rendered size of the checkbox row. */
    Size2d getSize() const override;
    /** @brief Returns the checkbox screen position. */
    Position2d getScreenPosition() const override { return position; }
    /** @brief Sets the checkbox screen position. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    /** @brief Returns the current label. */
    const std::string &getLabel() const { return label; }
    /** @brief Returns whether the checkbox is checked. */
    bool isChecked() const { return checked; }
    /** @brief Returns whether the cursor is currently hovering the checkbox. */
    bool isHovered() const { return hovered; }
    /** @brief Returns whether the checkbox is enabled. */
    bool isEnabled() const { return enabled; }
    /** @brief Returns a mutable local style override for this checkbox. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the label text. */
    Checkbox &setLabel(std::string newLabel);
    /** @brief Replaces the outer padding. */
    Checkbox &setPadding(Size2d newPadding);
    /** @brief Replaces the checkbox square size. */
    Checkbox &setBoxSize(float newBoxSize);
    /** @brief Replaces the gap between the box and label. */
    Checkbox &setSpacing(float newSpacing);
    /** @brief Replaces the explicit font size override. */
    Checkbox &setFontSize(float newFontSize);
    /** @brief Replaces the local style override. */
    Checkbox &setStyle(const graphite::UIStyle &newStyle);
    /** @brief Sets the checked state directly. */
    Checkbox &setChecked(bool newChecked);
    /** @brief Enables or disables interaction. */
    Checkbox &setEnabled(bool newEnabled);
    /** @brief Registers the callback invoked when the checkbox toggles. */
    Checkbox &setOnToggle(ToggleCallback callback);
    /** @brief Flips the checked state and emits a toggle event. */
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
