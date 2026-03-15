#ifndef GRAPHITE_STYLE_H
#define GRAPHITE_STYLE_H

#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

struct Font;

namespace graphite {

enum class UIStyleState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled,
    Checked,
};

struct UIStyleStateSnapshot {
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool disabled = false;
    bool checked = false;
};

struct UIStyleVariant {
    std::optional<Size2d> paddingValue;
    std::optional<float> cornerRadiusValue;
    std::optional<float> borderWidthValue;
    std::optional<Color> backgroundColorValue;
    std::optional<Color> borderColorValue;
    std::optional<Color> foregroundColorValue;
    std::optional<Color> tintColorValue;
    std::optional<const Font *> fontValue;
    std::optional<float> fontSizeValue;

    UIStyleVariant &padding(Size2d value);
    UIStyleVariant &padding(float horizontal, float vertical);
    UIStyleVariant &cornerRadius(float value);
    UIStyleVariant &background(const Color &value);
    UIStyleVariant &border(float width, const Color &color);
    UIStyleVariant &foreground(const Color &value);
    UIStyleVariant &tint(const Color &value);
    UIStyleVariant &font(const Font &value);
    UIStyleVariant &fontSize(float value);
};

struct UIResolvedStyle {
    Size2d padding{.width = 0.0f, .height = 0.0f};
    float cornerRadius = 0.0f;
    float borderWidth = 0.0f;
    Color backgroundColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    Color borderColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    Color foregroundColor = Color::white();
    Color tintColor = Color::white();
    const Font *font = nullptr;
    float fontSize = 0.0f;
};

class UIStyle {
  public:
    UIStyleVariant &normal();
    UIStyleVariant &hovered();
    UIStyleVariant &pressed();
    UIStyleVariant &focused();
    UIStyleVariant &disabled();
    UIStyleVariant &checked();
    UIStyleVariant &variant(UIStyleState state);

    const UIStyleVariant &normal() const;
    const UIStyleVariant &hovered() const;
    const UIStyleVariant &pressed() const;
    const UIStyleVariant &focused() const;
    const UIStyleVariant &disabled() const;
    const UIStyleVariant &checked() const;
    const UIStyleVariant &variant(UIStyleState state) const;

  private:
    UIStyleVariant normalVariant;
    UIStyleVariant hoveredVariant;
    UIStyleVariant pressedVariant;
    UIStyleVariant focusedVariant;
    UIStyleVariant disabledVariant;
    UIStyleVariant checkedVariant;
};

class Theme {
  public:
    UIStyle text;
    UIStyle image;
    UIStyle textField;
    UIStyle button;
    UIStyle checkbox;
    UIStyle row;
    UIStyle column;
    UIStyle stack;

    static Theme &current();
    static void set(const Theme &theme);
    static void reset();
};

struct BoxRendererData {
    std::shared_ptr<opal::DrawingState> vao = nullptr;
    std::shared_ptr<opal::Buffer> vertexBuffer = nullptr;
    std::size_t vertexBufferCapacity = 0;
    glm::mat4 projection{1.0f};
    ShaderProgram shader;
};

UIResolvedStyle resolveStyle(const UIStyle &fallbackStyle,
                             const UIStyle *themeStyle,
                             const UIStyle *overrideStyle = nullptr,
                             const UIStyleStateSnapshot &state = {});

float resolveTextScale(const Font &font, float fontSize = 0.0f);
float measureCharacterWidth(const Font &font, char ch, float fontSize = 0.0f);
float measureTextWidth(const Font &font, const std::string &text,
                       float fontSize = 0.0f);
float measureTextWidth(const Font &font, const std::string &text,
                       std::size_t start, std::size_t end,
                       float fontSize = 0.0f);
float getFontAscent(const Font &font, float fontSize = 0.0f);
float getFontDescent(const Font &font, float fontSize = 0.0f);
float getLineHeight(const Font &font, float fontSize = 0.0f);
std::string sanitizeText(const Font &font, const std::string &input);

void initializeBoxRenderer(BoxRendererData &renderer, uint objectId);
void renderStyledBox(BoxRendererData &renderer, uint objectId,
                     const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
                     Position2d position, Size2d size,
                     const UIResolvedStyle &style, uint textureId = 0);

} // namespace graphite

#endif // GRAPHITE_STYLE_H
