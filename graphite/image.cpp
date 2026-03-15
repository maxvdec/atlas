#include "graphite/image.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <cstddef>
#include <vector>

namespace {

struct ImageVertex {
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

uint getTextureId(const Texture &texture) {
    if (texture.id != 0) {
        return texture.id;
    }
    if (texture.texture != nullptr) {
        return texture.texture->textureID;
    }
    return 0;
}

std::vector<opal::VertexAttributeBinding>
makeImageBindings(const std::shared_ptr<opal::Buffer> &vertexBuffer) {
    opal::VertexAttribute positionAttribute{
        .name = "aPos",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 3,
        .stride = static_cast<uint>(sizeof(ImageVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute colorAttribute{
        .name = "aColor",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(offsetof(ImageVertex, r)),
        .location = 1,
        .normalized = false,
        .size = 4,
        .stride = static_cast<uint>(sizeof(ImageVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute texCoordAttribute{
        .name = "aTexCoord",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(offsetof(ImageVertex, u)),
        .location = 2,
        .normalized = false,
        .size = 2,
        .stride = static_cast<uint>(sizeof(ImageVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};

    return {{positionAttribute, vertexBuffer},
            {colorAttribute, vertexBuffer},
            {texCoordAttribute, vertexBuffer}};
}

} // namespace

Size2d Image::getSize() const {
    float width = size.width > 0.0f
                      ? size.width
                      : static_cast<float>(texture.creationData.width);
    float height = size.height > 0.0f
                       ? size.height
                       : static_cast<float>(texture.creationData.height);
    return {.width = width, .height = height};
}

void Image::setTexture(Texture newTexture) { texture = std::move(newTexture); }

void Image::setSize(Size2d newSize) { size = newSize; }

void Image::initialize() {
    for (auto &component : components) {
        component->init();
    }

    Size2d framebufferSize = Window::mainWindow->getSize();
    int fbWidth = static_cast<int>(framebufferSize.width);
    int fbHeight = static_cast<int>(framebufferSize.height);

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                        sizeof(ImageVertex) * 6, nullptr,
                                        opal::MemoryUsageType::CPUToGPU, id);
    vao = opal::DrawingState::create(vertexBuffer);
    vao->setBuffers(vertexBuffer, nullptr);
    vao->configureAttributes(makeImageBindings(vertexBuffer));

    shader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Texture,
                                               AtlasFragmentShader::Texture);
}

void Image::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                   bool updatePipeline) {
    (void)updatePipeline;

    if (shader.shader == nullptr || vao == nullptr || vertexBuffer == nullptr) {
        initialize();
    }

    for (auto &component : components) {
        component->update(dt);
    }

    if (commandBuffer == nullptr) {
        atlas_error("Image::render requires a valid command buffer");
        return;
    }

    uint textureId = getTextureId(texture);
    if (textureId == 0) {
        return;
    }

    Size2d imageSize = getSize();
    if (imageSize.width <= 0.0f || imageSize.height <= 0.0f) {
        return;
    }

    static std::shared_ptr<opal::Pipeline> imagePipeline = nullptr;

    Size2d framebufferSize = Window::mainWindow->getSize();
    int fbWidth = static_cast<int>(framebufferSize.width);
    int fbHeight = static_cast<int>(framebufferSize.height);

    if (imagePipeline == nullptr) {
        imagePipeline = opal::Pipeline::create();
        std::vector<opal::VertexAttribute> attributes = {
            {.name = "aPos",
             .type = opal::VertexAttributeType::Float,
             .offset = 0,
             .location = 0,
             .normalized = false,
             .size = 3,
             .stride = static_cast<uint>(sizeof(ImageVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aColor",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<uint>(offsetof(ImageVertex, r)),
             .location = 1,
             .normalized = false,
             .size = 4,
             .stride = static_cast<uint>(sizeof(ImageVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aTexCoord",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<uint>(offsetof(ImageVertex, u)),
             .location = 2,
             .normalized = false,
             .size = 2,
             .stride = static_cast<uint>(sizeof(ImageVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0}};
        imagePipeline->setVertexAttributes(
            attributes,
            {.stride = static_cast<uint>(sizeof(ImageVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex});
        imagePipeline->setShaderProgram(shader.shader);
#ifdef VULKAN
        imagePipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        imagePipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        imagePipeline->setCullMode(opal::CullMode::None);
        imagePipeline->enableDepthTest(false);
        imagePipeline->enableDepthWrite(false);
        imagePipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        imagePipeline->enableBlending(true);
        imagePipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                    opal::BlendFunc::OneMinusSrcAlpha);
        imagePipeline->build();
    } else {
#ifdef VULKAN
        imagePipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        imagePipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        imagePipeline->setShaderProgram(shader.shader);
    }

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
    float left = position.x;
    float top = position.y;
    float right = left + imageSize.width;
    float bottom = top + imageSize.height;
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
    float left = position.x;
    float top = static_cast<float>(fbHeight) - position.y;
    float right = left + imageSize.width;
    float bottom = top - imageSize.height;
#endif

#ifdef METAL
    ImageVertex vertices[6] = {
        {left, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 0.0f},
        {right, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 0.0f},
        {right, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 1.0f},
        {left, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 0.0f},
        {right, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 1.0f},
        {left, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 1.0f}};
#else
    ImageVertex vertices[6] = {
        {left, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 1.0f},
        {right, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 1.0f},
        {right, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 0.0f},
        {left, top, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 1.0f},
        {right, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 1.0f, 0.0f},
        {left, bottom, 0.0f, tint.r, tint.g, tint.b, tint.a, 0.0f, 0.0f}};
#endif

    vertexBuffer->bind();
    vertexBuffer->updateData(0, sizeof(vertices), vertices);
    vertexBuffer->unbind();

    imagePipeline->enableBlending(true);
    imagePipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                opal::BlendFunc::OneMinusSrcAlpha);
    imagePipeline->enableDepthTest(false);
    imagePipeline->enableDepthWrite(false);
    commandBuffer->bindPipeline(imagePipeline);
    imagePipeline->setUniformMat4f("model", glm::mat4(1.0f));
    imagePipeline->setUniformMat4f("view", glm::mat4(1.0f));
    imagePipeline->setUniformMat4f("projection", projection);
    imagePipeline->setUniform1i("useTexture", 1);
    imagePipeline->setUniform1i("onlyTexture", 1);
    imagePipeline->setUniform1i("textureCount", 1);
    imagePipeline->bindTexture2D("texture1", textureId, 0, id);

    commandBuffer->bindDrawingState(vao);
    commandBuffer->draw(6, 1, 0, 0, id);
    commandBuffer->unbindDrawingState();

    imagePipeline->enableBlending(false);
    imagePipeline->enableDepthTest(true);
    imagePipeline->enableDepthWrite(true);
    imagePipeline->bind();
}
