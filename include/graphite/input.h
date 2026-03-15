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

class TextField : public UIObject {
  public:
    using ChangeCallback =
        std::function<void(const TextFieldChangeEvent &)>;

    std::string text;
    std::string placeholder;
    Font font;
    Position2d position;
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

    TextField &setText(std::string newText);
    TextField &setPlaceholder(std::string newPlaceholder);
    TextField &setPadding(Size2d newPadding);
    TextField &setMaximumWidth(float newMaximumWidth);
    TextField &setOnChange(ChangeCallback callback);
    void focus();
    void blur();

  private:
    std::size_t cursorIndex = 0;
    std::size_t scrollIndex = 0;
    bool focused = false;
    std::shared_ptr<opal::DrawingState> boxVao = nullptr;
    std::shared_ptr<opal::Buffer> boxVertexBuffer = nullptr;
    std::size_t boxVertexBufferCapacity = 0;
    glm::mat4 projection;
    ShaderProgram boxShader;
    Text visibleText;
    Text placeholderText;
    ChangeCallback onChange;

    void ensureCursorVisible(float availableWidth);
    void syncRenderers(float availableWidth);
    void processInput();
    void emitChange() const;
};

#endif // GRAPHITE_INPUT_H
