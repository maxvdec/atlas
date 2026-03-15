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

Checkbox::Checkbox(Font font, std::string label, bool checked,
                   Position2d position)
    : label(sanitizeLabel(font, label)), font(std::move(font)),
      position(position), checked(checked) {}

Size2d Checkbox::getSize() const {
    const float labelWidth = measureTextWidth(font, label);
    const float contentWidth =
        boxSize + (label.empty() ? 0.0f : spacing + labelWidth);
    const float contentHeight = std::max(boxSize, getLineHeight(font));
    return {.width = contentWidth + (padding.width * 2.0f),
            .height = contentHeight + (padding.height * 2.0f)};
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

void Checkbox::syncLabel() {
    labelText.font = font;
    labelText.color =
        enabled ? textColor
                : Color(textColor.r, textColor.g, textColor.b,
                        textColor.a * 0.55f);
    labelText.content = label;

    const Size2d size = getSize();
    const float contentTop =
        position.y + ((size.height - getLineHeight(font)) * 0.5f);
    const float contentLeft = position.x + padding.width + boxSize +
                              (label.empty() ? 0.0f : spacing);

    labelText.setScreenPosition({.x = contentLeft, .y = contentTop});
}

void Checkbox::initialize() {
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

void Checkbox::render(float dt,
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

    syncLabel();

    static std::shared_ptr<opal::Pipeline> checkboxPipeline = nullptr;

    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

    if (checkboxPipeline == nullptr) {
        checkboxPipeline = opal::Pipeline::create();
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
        checkboxPipeline->setVertexAttributes(
            attributes,
            {.stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex});
        checkboxPipeline->setShaderProgram(boxShader.shader);
#ifdef VULKAN
        checkboxPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        checkboxPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        checkboxPipeline->setCullMode(opal::CullMode::None);
        checkboxPipeline->enableDepthTest(false);
        checkboxPipeline->enableDepthWrite(false);
        checkboxPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        checkboxPipeline->enableBlending(true);
        checkboxPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                       opal::BlendFunc::OneMinusSrcAlpha);
        checkboxPipeline->build();
    } else {
#ifdef VULKAN
        checkboxPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        checkboxPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        checkboxPipeline->setShaderProgram(boxShader.shader);
    }

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
    const float boxLeft = position.x + padding.width;
    const float boxTop = position.y + ((size.height - boxSize) * 0.5f);
    const float boxRight = boxLeft + boxSize;
    const float boxBottom = boxTop + boxSize;
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
    const float boxLeft = position.x + padding.width;
    const float boxTop =
        static_cast<float>(fbHeight) - (position.y + ((size.height - boxSize) * 0.5f));
    const float boxRight = boxLeft + boxSize;
    const float boxBottom = boxTop - boxSize;
#endif

    Color fillColor = hovered ? hoverBoxBackgroundColor : boxBackgroundColor;
    Color outlineColor = (hovered || checked) ? activeBorderColor : borderColor;
    Color activeColor = checkColor;

    if (!enabled) {
        fillColor = Color(fillColor.r, fillColor.g, fillColor.b,
                          fillColor.a * 0.55f);
        outlineColor = Color(outlineColor.r, outlineColor.g, outlineColor.b,
                             outlineColor.a * 0.55f);
        activeColor = Color(activeColor.r, activeColor.g, activeColor.b,
                            activeColor.a * 0.55f);
    }

    std::vector<BoxVertex> vertices;
    vertices.reserve(36);

    appendQuad(vertices, boxLeft, boxTop, boxRight, boxBottom, fillColor);
    appendQuad(vertices, boxLeft, boxTop, boxRight, boxTop + borderThickness,
               outlineColor);
    appendQuad(vertices, boxLeft, boxBottom - borderThickness, boxRight,
               boxBottom, outlineColor);
    appendQuad(vertices, boxLeft, boxTop, boxLeft + borderThickness, boxBottom,
               outlineColor);
    appendQuad(vertices, boxRight - borderThickness, boxTop, boxRight,
               boxBottom, outlineColor);

    if (checked) {
        const float inset = std::max(5.0f, boxSize * 0.22f);
        appendQuad(vertices, boxLeft + inset, boxTop + inset, boxRight - inset,
                   boxBottom - inset, activeColor);
    }

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

    checkboxPipeline->enableBlending(true);
    checkboxPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                   opal::BlendFunc::OneMinusSrcAlpha);
    checkboxPipeline->enableDepthTest(false);
    checkboxPipeline->enableDepthWrite(false);
    commandBuffer->bindPipeline(checkboxPipeline);
    checkboxPipeline->setUniformMat4f("model", glm::mat4(1.0f));
    checkboxPipeline->setUniformMat4f("view", glm::mat4(1.0f));
    checkboxPipeline->setUniformMat4f("projection", projection);
    checkboxPipeline->setUniform1i("useTexture", 0);
    checkboxPipeline->setUniform1i("onlyTexture", 0);
    checkboxPipeline->setUniform1i("textureCount", 0);
    commandBuffer->bindDrawingState(boxVao);
    commandBuffer->draw(static_cast<uint>(vertices.size()), 1, 0, 0, id);
    commandBuffer->unbindDrawingState();

    if (!label.empty()) {
        labelText.render(dt, commandBuffer, updatePipeline);
    }

    checkboxPipeline->enableBlending(false);
    checkboxPipeline->enableDepthTest(true);
    checkboxPipeline->enableDepthWrite(true);
    checkboxPipeline->bind();
}
