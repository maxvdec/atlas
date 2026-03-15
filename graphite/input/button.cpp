#include "graphite/input.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <algorithm>
#include <cstddef>
#include <vector>

namespace {

constexpr float textScale = 2.0f;
constexpr float borderThickness = 2.0f;

struct BoxVertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float a;
    float u;
    float v;
};

float measureCharacterWidth(const Font &font, char ch) {
    auto it = font.atlas.find(ch);
    if (it == font.atlas.end()) {
        return 0.0f;
    }
    return static_cast<float>(it->second.advance >> 6) * textScale;
}

float measureTextWidth(const Font &font, const std::string &text) {
    float width = 0.0f;
    for (char ch : text) {
        width += measureCharacterWidth(font, ch);
    }
    return width;
}

float getFontAscent(const Font &font) {
    float ascent = 0.0f;
    for (const auto &entry : font.atlas) {
        ascent = std::max(ascent, entry.second.bearing.y * textScale);
    }
    return ascent;
}

float getFontDescent(const Font &font) {
    float descent = 0.0f;
    for (const auto &entry : font.atlas) {
        descent = std::max(
            descent,
            (entry.second.size.height - entry.second.bearing.y) * textScale);
    }
    return descent;
}

float getLineHeight(const Font &font) {
    float height = getFontAscent(font) + getFontDescent(font);
    if (height <= 0.0f) {
        height = static_cast<float>(font.size) * textScale;
    }
    return height;
}

std::string sanitizeLabel(const Font &font, const std::string &input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char raw : input) {
        const char ch = static_cast<char>(raw);
        if (ch == '\n' || ch == '\r' || ch == '\t' || raw < 32) {
            continue;
        }
        if (font.atlas.contains(ch)) {
            result.push_back(ch);
        }
    }
    return result;
}

void appendQuad(std::vector<BoxVertex> &vertices, float left, float top,
                float right, float bottom, const Color &color) {
    vertices.push_back(
        {left, top, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 0.0f});
    vertices.push_back(
        {right, top, 0.0f, color.r, color.g, color.b, color.a, 1.0f, 0.0f});
    vertices.push_back({right, bottom, 0.0f, color.r, color.g, color.b,
                        color.a, 1.0f, 1.0f});
    vertices.push_back(
        {left, top, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 0.0f});
    vertices.push_back({right, bottom, 0.0f, color.r, color.g, color.b,
                        color.a, 1.0f, 1.0f});
    vertices.push_back({left, bottom, 0.0f, color.r, color.g, color.b, color.a,
                        0.0f, 1.0f});
}

std::vector<opal::VertexAttributeBinding>
makeBoxBindings(const std::shared_ptr<opal::Buffer> &vertexBuffer) {
    opal::VertexAttribute positionAttribute{
        .name = "aPos",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 3,
        .stride = static_cast<uint>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute colorAttribute{
        .name = "aColor",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(offsetof(BoxVertex, r)),
        .location = 1,
        .normalized = false,
        .size = 4,
        .stride = static_cast<uint>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute texCoordAttribute{
        .name = "aTexCoord",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(offsetof(BoxVertex, u)),
        .location = 2,
        .normalized = false,
        .size = 2,
        .stride = static_cast<uint>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};

    return {{positionAttribute, vertexBuffer},
            {colorAttribute, vertexBuffer},
            {texCoordAttribute, vertexBuffer}};
}

} // namespace

Button::Button(Font font, std::string label, Position2d position)
    : label(sanitizeLabel(font, label)), font(std::move(font)),
      position(position) {}

Size2d Button::getSize() const {
    const float width = std::max(minimumSize.width,
                                 measureTextWidth(font, label) +
                                     (padding.width * 2.0f));
    const float height = std::max(minimumSize.height,
                                  getLineHeight(font) +
                                      (padding.height * 2.0f));
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

void Button::syncLabel() {
    labelText.font = font;
    labelText.color =
        enabled ? textColor
                : Color(textColor.r, textColor.g, textColor.b,
                        textColor.a * 0.55f);
    labelText.content = label;

    const Size2d size = getSize();
    const float contentTop =
        position.y + ((size.height - getLineHeight(font)) * 0.5f);
    const float contentLeft =
        position.x + ((size.width - measureTextWidth(font, label)) * 0.5f);

    labelText.setScreenPosition({.x = contentLeft, .y = contentTop});
}

void Button::initialize() {
    for (auto &component : components) {
        component->init();
    }

    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    boxVertexBufferCapacity = sizeof(BoxVertex) * 36;
    boxVertexBuffer = opal::Buffer::create(
        opal::BufferUsage::VertexBuffer, boxVertexBufferCapacity, nullptr,
        opal::MemoryUsageType::CPUToGPU, id);
    boxVao = opal::DrawingState::create(boxVertexBuffer);
    boxVao->setBuffers(boxVertexBuffer, nullptr);
    boxVao->configureAttributes(makeBoxBindings(boxVertexBuffer));

    boxShader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Texture,
                                                  AtlasFragmentShader::Texture);
}

void Button::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    (void)updatePipeline;

    if (boxShader.shader == nullptr || boxVao == nullptr ||
        boxVertexBuffer == nullptr) {
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

    syncLabel();

    static std::shared_ptr<opal::Pipeline> buttonPipeline = nullptr;

    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

    if (buttonPipeline == nullptr) {
        buttonPipeline = opal::Pipeline::create();
        std::vector<opal::VertexAttribute> attributes = {
            {.name = "aPos",
             .type = opal::VertexAttributeType::Float,
             .offset = 0,
             .location = 0,
             .normalized = false,
             .size = 3,
             .stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aColor",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<uint>(offsetof(BoxVertex, r)),
             .location = 1,
             .normalized = false,
             .size = 4,
             .stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aTexCoord",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<uint>(offsetof(BoxVertex, u)),
             .location = 2,
             .normalized = false,
             .size = 2,
             .stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0}};
        buttonPipeline->setVertexAttributes(
            attributes,
            {.stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex});
        buttonPipeline->setShaderProgram(boxShader.shader);
#ifdef VULKAN
        buttonPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        buttonPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        buttonPipeline->setCullMode(opal::CullMode::None);
        buttonPipeline->enableDepthTest(false);
        buttonPipeline->enableDepthWrite(false);
        buttonPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        buttonPipeline->enableBlending(true);
        buttonPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                     opal::BlendFunc::OneMinusSrcAlpha);
        buttonPipeline->build();
    } else {
#ifdef VULKAN
        buttonPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        buttonPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        buttonPipeline->setShaderProgram(boxShader.shader);
    }

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
    const float left = position.x;
    const float top = position.y;
    const float right = left + size.width;
    const float bottom = top + size.height;
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
    const float left = position.x;
    const float top = static_cast<float>(fbHeight) - position.y;
    const float right = left + size.width;
    const float bottom = top - size.height;
#endif

    Color fillColor = backgroundColor;
    Color outlineColor = borderColor;

    if (!enabled) {
        fillColor = Color(fillColor.r, fillColor.g, fillColor.b,
                          fillColor.a * 0.55f);
        outlineColor = Color(outlineColor.r, outlineColor.g, outlineColor.b,
                             outlineColor.a * 0.55f);
    } else if (pressed) {
        fillColor = pressedBackgroundColor;
        outlineColor = hoverBorderColor;
    } else if (hovered) {
        fillColor = hoverBackgroundColor;
        outlineColor = hoverBorderColor;
    }

    std::vector<BoxVertex> vertices;
    vertices.reserve(30);

    appendQuad(vertices, left, top, right, bottom, fillColor);
    appendQuad(vertices, left, top, right, top + borderThickness, outlineColor);
    appendQuad(vertices, left, bottom - borderThickness, right, bottom,
               outlineColor);
    appendQuad(vertices, left, top, left + borderThickness, bottom,
               outlineColor);
    appendQuad(vertices, right - borderThickness, top, right, bottom,
               outlineColor);

    const std::size_t requiredBytes = vertices.size() * sizeof(BoxVertex);
    if (requiredBytes > boxVertexBufferCapacity) {
        boxVertexBufferCapacity = requiredBytes;
        boxVertexBuffer = opal::Buffer::create(
            opal::BufferUsage::VertexBuffer, boxVertexBufferCapacity, nullptr,
            opal::MemoryUsageType::CPUToGPU, id);
        boxVao->setBuffers(boxVertexBuffer, nullptr);
        boxVao->configureAttributes(makeBoxBindings(boxVertexBuffer));
    }

    boxVertexBuffer->bind();
    boxVertexBuffer->updateData(0, requiredBytes, vertices.data());
    boxVertexBuffer->unbind();

    buttonPipeline->enableBlending(true);
    buttonPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                 opal::BlendFunc::OneMinusSrcAlpha);
    buttonPipeline->enableDepthTest(false);
    buttonPipeline->enableDepthWrite(false);
    commandBuffer->bindPipeline(buttonPipeline);
    buttonPipeline->setUniformMat4f("model", glm::mat4(1.0f));
    buttonPipeline->setUniformMat4f("view", glm::mat4(1.0f));
    buttonPipeline->setUniformMat4f("projection", projection);
    buttonPipeline->setUniform1i("useTexture", 0);
    buttonPipeline->setUniform1i("onlyTexture", 0);
    buttonPipeline->setUniform1i("textureCount", 0);
    commandBuffer->bindDrawingState(boxVao);
    commandBuffer->draw(static_cast<uint>(vertices.size()), 1, 0, 0, id);
    commandBuffer->unbindDrawingState();

    if (!label.empty()) {
        labelText.render(dt, commandBuffer, updatePipeline);
    }

    buttonPipeline->enableBlending(false);
    buttonPipeline->enableDepthTest(true);
    buttonPipeline->enableDepthWrite(true);
    buttonPipeline->bind();
}
