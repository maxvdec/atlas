#ifndef OPAL_METAL_STATE_H
#define OPAL_METAL_STATE_H

#ifdef METAL

#include "opal/opal.h"
#include <Metal/Metal.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace opal::metal {

struct FieldType {
    size_t size = 0;
    size_t alignment = 1;
    bool isStruct = false;
    std::string structName;
};

struct StructField {
    std::string name;
    FieldType type;
    size_t offset = 0;
    size_t stride = 0;
    size_t arrayCount = 1;
};

struct StructLayout {
    std::string name;
    size_t size = 0;
    size_t alignment = 1;
    std::vector<StructField> fields;
};

struct BufferBinding {
    uint32_t index = 0;
    std::string structName;
    std::string instanceName;
    bool vertexStage = false;
    bool fragmentStage = false;
};

struct UniformLocation {
    uint32_t bufferIndex = 0;
    size_t offset = 0;
    size_t size = 0;
    bool vertexStage = false;
    bool fragmentStage = false;
};

struct ContextState {
    CA::MetalLayer *layer = nullptr;
};

struct DeviceState {
    Context *context = nullptr;
    MTL::Device *device = nullptr;
    MTL::CommandQueue *queue = nullptr;
    CA::MetalDrawable *drawable = nullptr;
    std::shared_ptr<Texture> brightTexture = nullptr;
    std::shared_ptr<Texture> depthTexture = nullptr;
    int drawableWidth = 0;
    int drawableHeight = 0;
};

struct BufferState {
    MTL::Buffer *buffer = nullptr;
    size_t size = 0;
};

struct TextureState {
    MTL::Texture *texture = nullptr;
    MTL::SamplerState *sampler = nullptr;
    TextureWrapMode wrapS = TextureWrapMode::Repeat;
    TextureWrapMode wrapT = TextureWrapMode::Repeat;
    TextureWrapMode wrapR = TextureWrapMode::Repeat;
    TextureFilterMode minFilter = TextureFilterMode::Linear;
    TextureFilterMode magFilter = TextureFilterMode::Linear;
    glm::vec4 borderColor = glm::vec4(0.0f);
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::Rgba8;
    TextureDataFormat dataFormat = TextureDataFormat::Rgba;
    int width = 0;
    int height = 0;
    int depth = 1;
    int samples = 1;
    uint32_t handle = 0;
};

struct ShaderState {
    MTL::Library *library = nullptr;
    MTL::Function *function = nullptr;
};

struct ProgramState {
    MTL::Function *vertexFunction = nullptr;
    MTL::Function *fragmentFunction = nullptr;
    std::unordered_map<std::string, StructLayout> layouts;
    std::vector<BufferBinding> bindings;
    std::unordered_map<uint32_t, size_t> bindingSize;
    std::unordered_map<std::string, std::vector<UniformLocation>>
        uniformResolutionCache;
};

struct PipelineState {
    MTL::VertexDescriptor *vertexDescriptor = nullptr;
    MTL::DepthStencilState *depthStencilState = nullptr;
    std::unordered_map<std::string, MTL::RenderPipelineState *>
        renderPipelineCache;
    std::unordered_map<uint32_t, std::vector<uint8_t>> uniformData;
    std::unordered_map<uint32_t, MTL::Buffer *> uniformBuffers;
    std::unordered_map<int, std::shared_ptr<Texture>> texturesByUnit;
    MTL::PrimitiveType primitiveType = MTL::PrimitiveTypeTriangle;
    MTL::CullMode cullMode = MTL::CullModeBack;
    MTL::Winding frontFace = MTL::WindingCounterClockwise;
    MTL::TriangleFillMode fillMode = MTL::TriangleFillModeFill;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    MTL::CompareFunction depthCompare = MTL::CompareFunctionLess;
    bool blendingEnabled = false;
    MTL::BlendFactor blendSrc = MTL::BlendFactorOne;
    MTL::BlendFactor blendDst = MTL::BlendFactorZero;
    MTL::BlendOperation blendOp = MTL::BlendOperationAdd;
    bool polygonOffsetEnabled = false;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 800;
    int viewportHeight = 600;
};

struct FramebufferState {
    bool placeholder = false;
};

struct CommandBufferState {
    NS::AutoreleasePool *autoreleasePool = nullptr;
    MTL::CommandBuffer *commandBuffer = nullptr;
    MTL::RenderCommandEncoder *encoder = nullptr;
    MTL::RenderPassDescriptor *passDescriptor = nullptr;
    CA::MetalDrawable *drawable = nullptr;
    std::array<MTL::Texture *, 32> boundVertexTextures = {};
    std::array<MTL::Texture *, 32> boundFragmentTextures = {};
    std::array<MTL::SamplerState *, 32> boundVertexSamplers = {};
    std::array<MTL::SamplerState *, 32> boundFragmentSamplers = {};
    bool textureBindingsInitialized = false;
    bool needsPresent = false;
    bool hasDraw = false;
    bool clearColorPending = false;
    bool clearDepthPending = false;
};

ContextState &contextState(Context *context);
DeviceState &deviceState(Device *device);
BufferState &bufferState(Buffer *buffer);
TextureState &textureState(Texture *texture);
ShaderState &shaderState(Shader *shader);
ProgramState &programState(ShaderProgram *program);
PipelineState &pipelineState(Pipeline *pipeline);
FramebufferState &framebufferState(Framebuffer *framebuffer);
CommandBufferState &commandBufferState(CommandBuffer *commandBuffer);
void releaseContextState(Context *context);
void releaseDeviceState(Device *device);
void releaseBufferState(Buffer *buffer);
void releaseTextureState(Texture *texture);
void releaseShaderState(Shader *shader);
void releaseProgramState(ShaderProgram *program);
void releasePipelineState(Pipeline *pipeline);
void releaseFramebufferState(Framebuffer *framebuffer);
void releaseCommandBufferState(CommandBuffer *commandBuffer);

uint32_t registerTextureHandle(const std::shared_ptr<Texture> &texture);
std::shared_ptr<Texture> getTextureFromHandle(uint32_t handle);
uint32_t stageBindingKey(uint32_t index, bool fragmentStage);

MTL::PixelFormat textureFormatToPixelFormat(TextureFormat format);
MTL::TextureType textureTypeToMetal(TextureType type);
MTL::TextureUsage textureUsageFor(TextureType type, TextureFormat format);
bool isDepthFormat(TextureFormat format);

MTL::SamplerAddressMode wrapModeToAddressMode(TextureWrapMode mode);
void rebuildTextureSampler(Texture *texture, MTL::Device *device);

bool parseProgramLayouts(const std::string &vertexSource,
                         const std::string &fragmentSource,
                         ProgramState &programState);
std::vector<UniformLocation> resolveUniformLocations(ProgramState &programState,
                                                     const std::string &name);

std::string makePipelineKey(const std::array<MTL::PixelFormat, 8> &colors,
                            uint32_t colorCount, MTL::PixelFormat depthFormat,
                            MTL::PixelFormat stencilFormat,
                            uint32_t sampleCount);
uint32_t fragmentColorOutputCount(const std::string &fragmentSource);

} // namespace opal::metal

#endif

#endif
