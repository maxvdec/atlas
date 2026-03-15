#include "graphite/input.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <algorithm>
#include <cstddef>
#include <vector>

namespace {

constexpr float textScale = 2.0f;
constexpr float borderThickness = 2.0f;
constexpr float cursorWidth = 2.0f;

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

float measureTextWidth(const Font &font, const std::string &text,
                       std::size_t start, std::size_t end) {
    float width = 0.0f;
    const std::size_t clampedEnd = std::min(end, text.size());
    for (std::size_t i = start; i < clampedEnd; ++i) {
        width += measureCharacterWidth(font, text[i]);
    }
    return width;
}

float getFontAscent(const Font &font) {
    float ascent = 0.0f;
    for (const auto &entry : font.atlas) {
        const Character &character = entry.second;
        ascent = std::max(ascent, character.bearing.y * textScale);
    }
    return ascent;
}

float getFontDescent(const Font &font) {
    float descent = 0.0f;
    for (const auto &entry : font.atlas) {
        const Character &character = entry.second;
        descent = std::max(
            descent, (character.size.height - character.bearing.y) * textScale);
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

std::string sanitizeInput(const Font &font, const std::string &input) {
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

std::size_t fitVisibleEnd(const Font &font, const std::string &text,
                          std::size_t start, float availableWidth) {
    float width = 0.0f;
    std::size_t index = start;
    while (index < text.size()) {
        const float charWidth = measureCharacterWidth(font, text[index]);
        if (index > start && width + charWidth > availableWidth) {
            break;
        }
        width += charWidth;
        ++index;
        if (width > availableWidth) {
            break;
        }
    }
    return index;
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

TextField::TextField(Font font, float maximumWidth, Position2d position,
                     std::string text, std::string placeholder)
    : text(sanitizeInput(font, text)), placeholder(std::move(placeholder)),
      font(std::move(font)), position(position),
      maximumWidth(std::max(maximumWidth, 0.0f)) {
    cursorIndex = this->text.size();
}

Size2d TextField::getSize() const {
    const float width = std::max(maximumWidth, padding.width * 2.0f);
    const float height = std::max(getLineHeight(font) + (padding.height * 2.0f),
                                  padding.height * 2.0f);
    return {.width = width, .height = height};
}

TextField &TextField::setText(std::string newText) {
    const std::string sanitized = sanitizeInput(font, newText);
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

TextField &TextField::setOnChange(ChangeCallback callback) {
    onChange = std::move(callback);
    return *this;
}

void TextField::focus() {
    focused = true;
    if (Window::mainWindow != nullptr && !Window::mainWindow->isTextInputActive()) {
        Window::mainWindow->startTextInput();
    }
}

void TextField::blur() {
    focused = false;
    if (Window::mainWindow != nullptr && Window::mainWindow->isTextInputActive()) {
        Window::mainWindow->stopTextInput();
    }
}

void TextField::emitChange() const {
    if (onChange) {
        onChange({.text = text, .cursorIndex = cursorIndex, .focused = focused});
    }
}

void TextField::ensureCursorVisible(float availableWidth) {
    cursorIndex = std::min(cursorIndex, text.size());
    scrollIndex = std::min(scrollIndex, cursorIndex);

    while (scrollIndex < cursorIndex &&
           measureTextWidth(font, text, scrollIndex, cursorIndex) >
               availableWidth) {
        ++scrollIndex;
    }

    while (scrollIndex > 0 &&
           measureTextWidth(font, text, scrollIndex - 1, cursorIndex) <=
               availableWidth) {
        --scrollIndex;
    }
}

void TextField::syncRenderers(float availableWidth) {
    ensureCursorVisible(availableWidth);

    const std::size_t visibleEnd =
        fitVisibleEnd(font, text, scrollIndex, availableWidth);
    const std::string visibleValue =
        text.substr(scrollIndex, visibleEnd - scrollIndex);

    visibleText.font = font;
    visibleText.color = textColor;
    visibleText.content = visibleValue;

    placeholderText.font = font;
    placeholderText.color = placeholderColor;
    placeholderText.content = placeholder;

    const float lineHeight = getLineHeight(font);
    const float contentLeft = position.x + padding.width;
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
            const float contentLeft = position.x + padding.width;
            const float localX =
                std::max(0.0f, static_cast<float>(cursorX) - contentLeft);
            std::size_t nextCursor = scrollIndex;
            float accumulatedWidth = 0.0f;
            while (nextCursor < text.size()) {
                const float charWidth =
                    measureCharacterWidth(font, text[nextCursor]);
                if (localX < accumulatedWidth + (charWidth * 0.5f)) {
                    break;
                }
                accumulatedWidth += charWidth;
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
        std::max(0.0f, fieldSize.width - (padding.width * 2.0f));

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

    const std::string typed = sanitizeInput(font, window->getTextInput());
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

void TextField::render(float dt,
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
        atlas_error("TextField::render requires a valid command buffer");
        return;
    }

    processInput();

    const Size2d fieldSize = getSize();
    const float availableWidth =
        std::max(0.0f, fieldSize.width - (padding.width * 2.0f));
    syncRenderers(availableWidth);

    static std::shared_ptr<opal::Pipeline> colorPipeline = nullptr;

    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

    if (colorPipeline == nullptr) {
        colorPipeline = opal::Pipeline::create();
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
        colorPipeline->setVertexAttributes(
            attributes,
            {.stride = static_cast<uint>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex});
        colorPipeline->setShaderProgram(boxShader.shader);
#ifdef VULKAN
        colorPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        colorPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        colorPipeline->setCullMode(opal::CullMode::None);
        colorPipeline->enableDepthTest(false);
        colorPipeline->enableDepthWrite(false);
        colorPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        colorPipeline->enableBlending(true);
        colorPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                    opal::BlendFunc::OneMinusSrcAlpha);
        colorPipeline->build();
    } else {
#ifdef VULKAN
        colorPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        colorPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        colorPipeline->setShaderProgram(boxShader.shader);
    }

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
    const float left = position.x;
    const float top = position.y;
    const float right = left + fieldSize.width;
    const float bottom = top + fieldSize.height;
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
    const float left = position.x;
    const float top = static_cast<float>(fbHeight) - position.y;
    const float right = left + fieldSize.width;
    const float bottom = top - fieldSize.height;
#endif

    std::vector<BoxVertex> vertices;
    vertices.reserve(36);

    appendQuad(vertices, left, top, right, bottom, backgroundColor);

    const Color outlineColor = focused ? focusedBorderColor : borderColor;
    appendQuad(vertices, left, top, right, top + borderThickness, outlineColor);
    appendQuad(vertices, left, bottom - borderThickness, right, bottom,
               outlineColor);
    appendQuad(vertices, left, top, left + borderThickness, bottom,
               outlineColor);
    appendQuad(vertices, right - borderThickness, top, right, bottom,
               outlineColor);

    if (focused) {
        const float cursorX =
            position.x + padding.width +
            measureTextWidth(font, text, scrollIndex, cursorIndex);
        const float cursorTop = position.y + padding.height;
        const float cursorBottom = position.y + fieldSize.height - padding.height;
#if defined(VULKAN) || defined(METAL)
        appendQuad(vertices, cursorX, cursorTop, cursorX + cursorWidth,
                   cursorBottom, cursorColor);
#else
        appendQuad(vertices, cursorX, static_cast<float>(fbHeight) - cursorTop,
                   cursorX + cursorWidth,
                   static_cast<float>(fbHeight) - cursorBottom, cursorColor);
#endif
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

    colorPipeline->enableBlending(true);
    colorPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                opal::BlendFunc::OneMinusSrcAlpha);
    colorPipeline->enableDepthTest(false);
    colorPipeline->enableDepthWrite(false);
    commandBuffer->bindPipeline(colorPipeline);
    colorPipeline->setUniformMat4f("model", glm::mat4(1.0f));
    colorPipeline->setUniformMat4f("view", glm::mat4(1.0f));
    colorPipeline->setUniformMat4f("projection", projection);
    colorPipeline->setUniform1i("useTexture", 0);
    colorPipeline->setUniform1i("onlyTexture", 0);
    colorPipeline->setUniform1i("textureCount", 0);
    commandBuffer->bindDrawingState(boxVao);
    commandBuffer->draw(static_cast<uint>(vertices.size()), 1, 0, 0, id);
    commandBuffer->unbindDrawingState();

    if (!text.empty()) {
        visibleText.render(dt, commandBuffer, updatePipeline);
    } else if (!placeholder.empty()) {
        placeholderText.render(dt, commandBuffer, updatePipeline);
    }

    colorPipeline->enableBlending(false);
    colorPipeline->enableDepthTest(true);
    colorPipeline->enableDepthWrite(true);
    colorPipeline->bind();
}
