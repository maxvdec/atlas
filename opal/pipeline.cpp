//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for the core renderer
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#ifdef METAL
#include "metal_state.h"
#endif
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

#ifdef VULKAN
namespace {
void logMissingUniformOnce(const std::string &name) {
    static std::unordered_set<std::string> missing;
    if (missing.insert(name).second) {
        atlas_warning(std::string("Vulkan uniform not found: ") + name);
    }
}
} // namespace
#endif

#ifdef METAL
namespace {

constexpr NS::UInteger kVertexStreamBufferIndex = 24;
constexpr NS::UInteger kInstanceStreamBufferIndex = 25;

template <typename T>
static inline T alignUp(T value, T alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

static MTL::BlendFactor toMetalBlendFactor(BlendFunc factor) {
    switch (factor) {
    case BlendFunc::Zero:
        return MTL::BlendFactorZero;
    case BlendFunc::One:
        return MTL::BlendFactorOne;
    case BlendFunc::SrcColor:
        return MTL::BlendFactorSourceColor;
    case BlendFunc::OneMinusSrcColor:
        return MTL::BlendFactorOneMinusSourceColor;
    case BlendFunc::DstColor:
        return MTL::BlendFactorDestinationColor;
    case BlendFunc::OneMinusDstColor:
        return MTL::BlendFactorOneMinusDestinationColor;
    case BlendFunc::SrcAlpha:
        return MTL::BlendFactorSourceAlpha;
    case BlendFunc::OneMinusSrcAlpha:
        return MTL::BlendFactorOneMinusSourceAlpha;
    case BlendFunc::DstAlpha:
        return MTL::BlendFactorDestinationAlpha;
    case BlendFunc::OneMinusDstAlpha:
        return MTL::BlendFactorOneMinusDestinationAlpha;
    default:
        return MTL::BlendFactorOne;
    }
}

static MTL::BlendOperation toMetalBlendOperation(BlendEquation equation) {
    switch (equation) {
    case BlendEquation::Add:
        return MTL::BlendOperationAdd;
    case BlendEquation::Subtract:
        return MTL::BlendOperationSubtract;
    case BlendEquation::ReverseSubtract:
        return MTL::BlendOperationReverseSubtract;
    case BlendEquation::Min:
        return MTL::BlendOperationMin;
    case BlendEquation::Max:
        return MTL::BlendOperationMax;
    default:
        return MTL::BlendOperationAdd;
    }
}

static MTL::CompareFunction toMetalCompare(CompareOp op) {
    switch (op) {
    case CompareOp::Never:
        return MTL::CompareFunctionNever;
    case CompareOp::Less:
        return MTL::CompareFunctionLess;
    case CompareOp::Equal:
        return MTL::CompareFunctionEqual;
    case CompareOp::LessEqual:
        return MTL::CompareFunctionLessEqual;
    case CompareOp::Greater:
        return MTL::CompareFunctionGreater;
    case CompareOp::NotEqual:
        return MTL::CompareFunctionNotEqual;
    case CompareOp::GreaterEqual:
        return MTL::CompareFunctionGreaterEqual;
    case CompareOp::Always:
        return MTL::CompareFunctionAlways;
    default:
        return MTL::CompareFunctionLess;
    }
}

static MTL::PrimitiveType toMetalPrimitive(PrimitiveStyle style) {
    switch (style) {
    case PrimitiveStyle::Points:
        return MTL::PrimitiveTypePoint;
    case PrimitiveStyle::Lines:
        return MTL::PrimitiveTypeLine;
    case PrimitiveStyle::LineStrip:
        return MTL::PrimitiveTypeLineStrip;
    case PrimitiveStyle::Triangles:
        return MTL::PrimitiveTypeTriangle;
    case PrimitiveStyle::TriangleStrip:
        return MTL::PrimitiveTypeTriangleStrip;
    case PrimitiveStyle::TriangleFan:
        return MTL::PrimitiveTypeTriangle;
    case PrimitiveStyle::Patches:
        return MTL::PrimitiveTypeTriangle;
    default:
        return MTL::PrimitiveTypeTriangle;
    }
}

static MTL::CullMode toMetalCull(CullMode mode) {
    switch (mode) {
    case CullMode::None:
        return MTL::CullModeNone;
    case CullMode::Front:
        return MTL::CullModeFront;
    case CullMode::Back:
        return MTL::CullModeBack;
    case CullMode::FrontAndBack:
        return MTL::CullModeNone;
    default:
        return MTL::CullModeBack;
    }
}

static MTL::Winding toMetalWinding(FrontFace face) {
    switch (face) {
    case FrontFace::Clockwise:
        return MTL::WindingCounterClockwise;
    case FrontFace::CounterClockwise:
        return MTL::WindingClockwise;
    default:
        return MTL::WindingClockwise;
    }
}

static MTL::TriangleFillMode toMetalFillMode(RasterizerMode mode) {
    switch (mode) {
    case RasterizerMode::Fill:
        return MTL::TriangleFillModeFill;
    case RasterizerMode::Line:
        return MTL::TriangleFillModeLines;
    case RasterizerMode::Point:
        return MTL::TriangleFillModeFill;
    default:
        return MTL::TriangleFillModeFill;
    }
}

static MTL::VertexFormat toMetalVertexFormat(VertexAttributeType type, uint size,
                                             bool normalized) {
    switch (type) {
    case VertexAttributeType::Float:
        if (size == 1)
            return MTL::VertexFormatFloat;
        if (size == 2)
            return MTL::VertexFormatFloat2;
        if (size == 3)
            return MTL::VertexFormatFloat3;
        if (size == 4)
            return MTL::VertexFormatFloat4;
        return MTL::VertexFormatFloat4;
    case VertexAttributeType::Int:
        if (size == 1)
            return MTL::VertexFormatInt;
        if (size == 2)
            return MTL::VertexFormatInt2;
        if (size == 3)
            return MTL::VertexFormatInt3;
        if (size == 4)
            return MTL::VertexFormatInt4;
        return MTL::VertexFormatInt4;
    case VertexAttributeType::UnsignedInt:
        if (size == 1)
            return MTL::VertexFormatUInt;
        if (size == 2)
            return MTL::VertexFormatUInt2;
        if (size == 3)
            return MTL::VertexFormatUInt3;
        if (size == 4)
            return MTL::VertexFormatUInt4;
        return MTL::VertexFormatUInt4;
    case VertexAttributeType::Short:
        if (size == 2)
            return normalized ? MTL::VertexFormatShort2Normalized
                              : MTL::VertexFormatShort2;
        if (size == 3)
            return normalized ? MTL::VertexFormatShort3Normalized
                              : MTL::VertexFormatShort3;
        if (size == 4)
            return normalized ? MTL::VertexFormatShort4Normalized
                              : MTL::VertexFormatShort4;
        return normalized ? MTL::VertexFormatShort2Normalized
                          : MTL::VertexFormatShort2;
    case VertexAttributeType::UnsignedShort:
        if (size == 2)
            return normalized ? MTL::VertexFormatUShort2Normalized
                              : MTL::VertexFormatUShort2;
        if (size == 3)
            return normalized ? MTL::VertexFormatUShort3Normalized
                              : MTL::VertexFormatUShort3;
        if (size == 4)
            return normalized ? MTL::VertexFormatUShort4Normalized
                              : MTL::VertexFormatUShort4;
        return normalized ? MTL::VertexFormatUShort2Normalized
                          : MTL::VertexFormatUShort2;
    case VertexAttributeType::Byte:
        if (size == 2)
            return normalized ? MTL::VertexFormatChar2Normalized
                              : MTL::VertexFormatChar2;
        if (size == 3)
            return normalized ? MTL::VertexFormatChar3Normalized
                              : MTL::VertexFormatChar3;
        if (size == 4)
            return normalized ? MTL::VertexFormatChar4Normalized
                              : MTL::VertexFormatChar4;
        return normalized ? MTL::VertexFormatCharNormalized
                          : MTL::VertexFormatChar;
    case VertexAttributeType::UnsignedByte:
        if (size == 2)
            return normalized ? MTL::VertexFormatUChar2Normalized
                              : MTL::VertexFormatUChar2;
        if (size == 3)
            return normalized ? MTL::VertexFormatUChar3Normalized
                              : MTL::VertexFormatUChar3;
        if (size == 4)
            return normalized ? MTL::VertexFormatUChar4Normalized
                              : MTL::VertexFormatUChar4;
        return normalized ? MTL::VertexFormatUCharNormalized
                          : MTL::VertexFormatUChar;
    case VertexAttributeType::Double:
        return MTL::VertexFormatFloat4;
    default:
        return MTL::VertexFormatFloat4;
    }
}

static void updateMetalUniform(Pipeline *pipeline, const std::string &name,
                               const void *data, size_t size,
                               bool clampToDeclaredSize) {
    if (pipeline == nullptr || pipeline->shaderProgram == nullptr || data == nullptr ||
        size == 0) {
        return;
    }

    auto &programState = metal::programState(pipeline->shaderProgram.get());
    auto locations = metal::resolveUniformLocations(programState, name);
    if (locations.empty()) {
        return;
    }

    auto &pipelineState = metal::pipelineState(pipeline);
    for (const auto &location : locations) {
        auto &bytes = pipelineState.uniformData[location.bufferIndex];

        size_t bindingSize = 0;
        auto bindingIt = programState.bindingSize.find(location.bufferIndex);
        if (bindingIt != programState.bindingSize.end()) {
            bindingSize = bindingIt->second;
        }

        size_t writeSize = clampToDeclaredSize ? std::min(size, location.size)
                                               : size;
        size_t requiredSize = location.offset + writeSize;
        requiredSize = std::max(requiredSize, bindingSize);
        if (bytes.size() < requiredSize) {
            bytes.resize(alignUp(requiredSize, static_cast<size_t>(16)), 0);
        }

        std::memcpy(bytes.data() + location.offset, data, writeSize);
    }
}

} // namespace
#endif

std::shared_ptr<Pipeline> Pipeline::create() {
    auto pipeline = std::make_shared<Pipeline>();
    return pipeline;
}

#ifdef VULKAN
std::vector<std::shared_ptr<CoreRenderPass>> RenderPass::cachedRenderPasses =
    {};
#endif

void Pipeline::setShaderProgram(std::shared_ptr<ShaderProgram> program) {
    this->shaderProgram = program;
}

void Pipeline::setVertexAttributes(
    const std::vector<VertexAttribute> &attributes,
    const VertexBinding &binding) {
    this->vertexAttributes = attributes;
    this->vertexBinding = binding;
}

void Pipeline::setPrimitiveStyle(PrimitiveStyle style) {
    this->primitiveStyle = style;
}

void Pipeline::setPatchVertices(int count) { this->patchVertices = count; }

void Pipeline::setViewport(int x, int y, int width, int height) {
    this->viewportX = x;
    this->viewportY = y;
    this->viewportWidth = width;
    this->viewportHeight = height;
#ifdef VULKAN
    this->vkViewport.x = static_cast<float>(x);
    this->vkViewport.y = static_cast<float>(y);
    this->vkViewport.width = static_cast<float>(width);
    this->vkViewport.height = static_cast<float>(height);
    this->vkViewport.minDepth = 0.0f;
    this->vkViewport.maxDepth = 1.0f;
#endif
}

void Pipeline::setRasterizerMode(RasterizerMode mode) {
    this->rasterizerMode = mode;
}

void Pipeline::setCullMode(CullMode mode) { this->cullMode = mode; }

void Pipeline::setFrontFace(FrontFace face) { this->frontFace = face; }

void Pipeline::enableDepthTest(bool enabled) {
    this->depthTestEnabled = enabled;
}

void Pipeline::setDepthCompareOp(CompareOp op) { this->depthCompareOp = op; }

void Pipeline::enableDepthWrite(bool enabled) {
    this->depthWriteEnabled = enabled;
}

void Pipeline::enableBlending(bool enabled) { this->blendingEnabled = enabled; }

void Pipeline::setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor) {
    this->blendSrcFactor = srcFactor;
    this->blendDstFactor = dstFactor;
}

void Pipeline::setBlendEquation(BlendEquation equation) {
    this->blendEquation = equation;
}

void Pipeline::enableMultisampling(bool enabled) {
    this->multisamplingEnabled = enabled;
}

void Pipeline::enablePolygonOffset(bool enabled) {
    this->polygonOffsetEnabled = enabled;
}

void Pipeline::setPolygonOffset(float factor, float units) {
    this->polygonOffsetFactor = factor;
    this->polygonOffsetUnits = units;
}

void Pipeline::enableClipDistance(int index, bool enabled) {
    if (enabled) {
        if (std::find(this->enabledClipDistances.begin(),
                      this->enabledClipDistances.end(),
                      index) == this->enabledClipDistances.end()) {
            this->enabledClipDistances.push_back(index);
        }
    } else {
        auto it = std::find(this->enabledClipDistances.begin(),
                            this->enabledClipDistances.end(), index);
        if (it != this->enabledClipDistances.end()) {
            this->enabledClipDistances.erase(it);
        }
    }
}

uint Pipeline::getGLBlendFactor(BlendFunc func) const {
    switch (func) {
    case BlendFunc::Zero:
        return GL_ZERO;
    case BlendFunc::One:
        return GL_ONE;
    case BlendFunc::SrcColor:
        return GL_SRC_COLOR;
    case BlendFunc::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFunc::DstColor:
        return GL_DST_COLOR;
    case BlendFunc::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFunc::SrcAlpha:
        return GL_SRC_ALPHA;
    case BlendFunc::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFunc::DstAlpha:
        return GL_DST_ALPHA;
    case BlendFunc::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        return GL_ONE;
    }
}

uint Pipeline::getGLBlendEquation(BlendEquation equation) const {
    switch (equation) {
    case BlendEquation::Add:
        return GL_FUNC_ADD;
    case BlendEquation::Subtract:
        return GL_FUNC_SUBTRACT;
    case BlendEquation::ReverseSubtract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case BlendEquation::Min:
        return GL_MIN;
    case BlendEquation::Max:
        return GL_MAX;
    default:
        return GL_FUNC_ADD;
    }
}

uint Pipeline::getGLCompareOp(CompareOp op) const {
    switch (op) {
    case CompareOp::Never:
        return GL_NEVER;
    case CompareOp::Less:
        return GL_LESS;
    case CompareOp::Equal:
        return GL_EQUAL;
    case CompareOp::LessEqual:
        return GL_LEQUAL;
    case CompareOp::Greater:
        return GL_GREATER;
    case CompareOp::NotEqual:
        return GL_NOTEQUAL;
    case CompareOp::GreaterEqual:
        return GL_GEQUAL;
    case CompareOp::Always:
        return GL_ALWAYS;
    default:
        return GL_LESS;
    }
}

uint Pipeline::getGLPrimitiveStyle(PrimitiveStyle style) const {
    switch (style) {
    case PrimitiveStyle::Points:
        return GL_POINTS;
    case PrimitiveStyle::Lines:
        return GL_LINES;
    case PrimitiveStyle::LineStrip:
        return GL_LINE_STRIP;
    case PrimitiveStyle::Triangles:
        return GL_TRIANGLES;
    case PrimitiveStyle::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case PrimitiveStyle::TriangleFan:
        return GL_TRIANGLE_FAN;
    case PrimitiveStyle::Patches:
        return GL_PATCHES;
    default:
        return GL_TRIANGLES;
    }
}

uint Pipeline::getGLRasterizerMode(RasterizerMode mode) const {
    switch (mode) {
    case RasterizerMode::Fill:
        return GL_FILL;
    case RasterizerMode::Line:
        return GL_LINE;
    case RasterizerMode::Point:
        return GL_POINT;
    default:
        return GL_FILL;
    }
}

uint Pipeline::getGLCullMode(CullMode mode) const {
    switch (mode) {
    case CullMode::None:
        return 0;
    case CullMode::Front:
        return GL_FRONT;
    case CullMode::Back:
        return GL_BACK;
    case CullMode::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        return GL_BACK;
    }
}

uint Pipeline::getGLFrontFace(FrontFace face) const {
    switch (face) {
    case FrontFace::Clockwise:
        return GL_CW;
    case FrontFace::CounterClockwise:
        return GL_CCW;
    default:
        return GL_CCW;
    }
}

uint Pipeline::getGLVertexAttributeType(VertexAttributeType type) const {
    switch (type) {
    case VertexAttributeType::Float:
        return GL_FLOAT;
    case VertexAttributeType::Int:
        return GL_INT;
    case VertexAttributeType::UnsignedInt:
        return GL_UNSIGNED_INT;
    case VertexAttributeType::Short:
        return GL_SHORT;
    case VertexAttributeType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case VertexAttributeType::Byte:
        return GL_BYTE;
    case VertexAttributeType::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    default:
        return GL_FLOAT;
    }
}

void Pipeline::build() {
#ifdef OPENGL
    (void)this; // Vertex layout applied explicitly per VAO.
#elif defined(VULKAN)
    this->buildPipelineLayout();
#elif defined(METAL)
    if (Device::globalInstance == nullptr) {
        throw std::runtime_error("Cannot build Metal pipeline without device");
    }
    auto &deviceState = metal::deviceState(Device::globalInstance);
    if (deviceState.device == nullptr) {
        throw std::runtime_error("Metal device is not initialized");
    }

    if (shaderProgram == nullptr) {
        throw std::runtime_error("Pipeline::build() requires a shader program");
    }

    auto &state = metal::pipelineState(this);

    state.primitiveType = toMetalPrimitive(this->primitiveStyle);
    state.cullMode = toMetalCull(this->cullMode);
    state.frontFace = toMetalWinding(this->frontFace);
    state.fillMode = toMetalFillMode(this->rasterizerMode);
    state.depthTestEnabled = this->depthTestEnabled;
    state.depthWriteEnabled = this->depthWriteEnabled;
    state.depthCompare = toMetalCompare(this->depthCompareOp);
    state.blendingEnabled = this->blendingEnabled;
    state.blendSrc = toMetalBlendFactor(this->blendSrcFactor);
    state.blendDst = toMetalBlendFactor(this->blendDstFactor);
    state.blendOp = toMetalBlendOperation(this->blendEquation);
    state.polygonOffsetEnabled = this->polygonOffsetEnabled;
    state.polygonOffsetFactor = this->polygonOffsetFactor;
    state.polygonOffsetUnits = this->polygonOffsetUnits;
    state.viewportX = this->viewportX;
    state.viewportY = this->viewportY;
    state.viewportWidth = this->viewportWidth;
    state.viewportHeight = this->viewportHeight;

    if (state.vertexDescriptor != nullptr) {
        state.vertexDescriptor->release();
    }
    state.vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    auto *layouts = state.vertexDescriptor->layouts();
    auto *attributes = state.vertexDescriptor->attributes();

    NS::UInteger instanceStride = 0;
    NS::UInteger instanceDivisor = 1;
    bool hasInstance = false;

    for (const auto &attribute : this->vertexAttributes) {
        auto *descriptor = attributes->object(attribute.location);
        if (descriptor == nullptr) {
            continue;
        }

        bool isInstance =
            attribute.inputRate == VertexBindingInputRate::Instance;
        NS::UInteger bufferIndex =
            isInstance ? kInstanceStreamBufferIndex : kVertexStreamBufferIndex;

        descriptor->setBufferIndex(bufferIndex);
        descriptor->setOffset(static_cast<NS::UInteger>(attribute.offset));
        descriptor->setFormat(toMetalVertexFormat(attribute.type, attribute.size,
                                                  attribute.normalized));

        if (isInstance) {
            hasInstance = true;
            instanceStride = std::max(instanceStride,
                                      static_cast<NS::UInteger>(attribute.stride));
            unsigned int divisor = attribute.divisor == 0 ? 1 : attribute.divisor;
            instanceDivisor = std::max(instanceDivisor,
                                       static_cast<NS::UInteger>(divisor));
        }
    }

    auto *vertexLayout = layouts->object(kVertexStreamBufferIndex);
    if (vertexLayout != nullptr) {
        vertexLayout->setStride(static_cast<NS::UInteger>(vertexBinding.stride));
        vertexLayout->setStepFunction(MTL::VertexStepFunctionPerVertex);
        vertexLayout->setStepRate(1);
    }

    if (hasInstance) {
        auto *instanceLayout = layouts->object(kInstanceStreamBufferIndex);
        if (instanceLayout != nullptr) {
            instanceLayout->setStride(instanceStride);
            instanceLayout->setStepFunction(MTL::VertexStepFunctionPerInstance);
            instanceLayout->setStepRate(instanceDivisor);
        }
    }

    MTL::DepthStencilDescriptor *depthDescriptor =
        MTL::DepthStencilDescriptor::alloc()->init();
    if (this->depthTestEnabled) {
        depthDescriptor->setDepthCompareFunction(
            toMetalCompare(this->depthCompareOp));
    } else {
        depthDescriptor->setDepthCompareFunction(MTL::CompareFunctionAlways);
    }
    depthDescriptor->setDepthWriteEnabled(this->depthWriteEnabled);
    if (state.depthStencilState != nullptr) {
        state.depthStencilState->release();
    }
    state.depthStencilState = deviceState.device->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();
#endif
}

void Pipeline::bind() {
#ifdef OPENGL
    if (this->shaderProgram == nullptr) {
        throw std::runtime_error(
            "Pipeline::bind() called but no shader program is set. "
            "Call setShaderProgram() or refreshPipeline() first.");
    }
    glUseProgram(this->shaderProgram->programID);

    glViewport(this->viewportX, this->viewportY, this->viewportWidth,
               this->viewportHeight);

    glPolygonMode(GL_FRONT_AND_BACK,
                  this->getGLRasterizerMode(this->rasterizerMode));

    if (this->cullMode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(this->getGLCullMode(this->cullMode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    glFrontFace(this->getGLFrontFace(this->frontFace));

    if (this->depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(this->getGLCompareOp(this->depthCompareOp));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(this->depthWriteEnabled ? GL_TRUE : GL_FALSE);

    if (this->blendingEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(this->getGLBlendFactor(this->blendSrcFactor),
                    this->getGLBlendFactor(this->blendDstFactor));
        glBlendEquation(this->getGLBlendEquation(this->blendEquation));
    } else {
        glDisable(GL_BLEND);
    }

    if (this->multisamplingEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    if (this->polygonOffsetEnabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(this->polygonOffsetFactor, this->polygonOffsetUnits);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // Handle clip distances (up to 8 supported)
    for (int i = 0; i < 8; ++i) {
        bool shouldEnable = std::find(this->enabledClipDistances.begin(),
                                      this->enabledClipDistances.end(),
                                      i) != this->enabledClipDistances.end();
        if (shouldEnable) {
            glEnable(GL_CLIP_DISTANCE0 + i);
        } else {
            glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }
#elif defined(METAL)
    auto &state = metal::pipelineState(this);
    state.primitiveType = toMetalPrimitive(this->primitiveStyle);
    state.cullMode = toMetalCull(this->cullMode);
    state.frontFace = toMetalWinding(this->frontFace);
    state.fillMode = toMetalFillMode(this->rasterizerMode);
    state.depthTestEnabled = this->depthTestEnabled;
    state.depthWriteEnabled = this->depthWriteEnabled;
    state.depthCompare = toMetalCompare(this->depthCompareOp);
    state.blendingEnabled = this->blendingEnabled;
    state.blendSrc = toMetalBlendFactor(this->blendSrcFactor);
    state.blendDst = toMetalBlendFactor(this->blendDstFactor);
    state.blendOp = toMetalBlendOperation(this->blendEquation);
    state.polygonOffsetEnabled = this->polygonOffsetEnabled;
    state.polygonOffsetFactor = this->polygonOffsetFactor;
    state.polygonOffsetUnits = this->polygonOffsetUnits;
    state.viewportX = this->viewportX;
    state.viewportY = this->viewportY;
    state.viewportWidth = this->viewportWidth;
    state.viewportHeight = this->viewportHeight;

    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        if (deviceState.device != nullptr) {
            MTL::DepthStencilDescriptor *depthDescriptor =
                MTL::DepthStencilDescriptor::alloc()->init();
            if (this->depthTestEnabled) {
                depthDescriptor->setDepthCompareFunction(
                    toMetalCompare(this->depthCompareOp));
            } else {
                depthDescriptor->setDepthCompareFunction(
                    MTL::CompareFunctionAlways);
            }
            depthDescriptor->setDepthWriteEnabled(this->depthWriteEnabled);
            if (state.depthStencilState != nullptr) {
                state.depthStencilState->release();
            }
            state.depthStencilState =
                deviceState.device->newDepthStencilState(depthDescriptor);
            depthDescriptor->release();
        }
    }
#endif
}

bool Pipeline::operator==(std::shared_ptr<Pipeline> pipeline) const {
    if (this->primitiveStyle != pipeline->primitiveStyle) {
        return false;
    }
    if (this->rasterizerMode != pipeline->rasterizerMode) {
        return false;
    }
    if (this->cullMode != pipeline->cullMode) {
        return false;
    }
    if (this->frontFace != pipeline->frontFace) {
        return false;
    }
    if (this->blendingEnabled != pipeline->blendingEnabled) {
        return false;
    }
    if (this->blendSrcFactor != pipeline->blendSrcFactor) {
        return false;
    }
    if (this->blendDstFactor != pipeline->blendDstFactor) {
        return false;
    }
    if (this->depthTestEnabled != pipeline->depthTestEnabled) {
        return false;
    }
    if (this->depthCompareOp != pipeline->depthCompareOp) {
        return false;
    }
    if (this->shaderProgram != pipeline->shaderProgram) {
        return false;
    }
    if (this->vertexAttributes != pipeline->vertexAttributes) {
        return false;
    }
    if (this->vertexBinding.inputRate != pipeline->vertexBinding.inputRate) {
        return false;
    }
    if (this->vertexBinding.stride != pipeline->vertexBinding.stride) {
        return false;
    }
    return true;
}

void Pipeline::setUniform1f(const std::string &name, float v0) {
#ifdef OPENGL
    glUniform1f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    if (!info->isBuffer) {
        // Push constant
        updatePushConstant(info->offset, &v0, sizeof(float));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &v0,
                          sizeof(float));
    }
#elif defined(METAL)
    updateMetalUniform(this, name, &v0, sizeof(float), true);
#endif
}

void Pipeline::setUniformMat4f(const std::string &name,
                               const glm::mat4 &matrix) {
#ifdef OPENGL
    glUniformMatrix4fv(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), 1,
        GL_FALSE, &matrix[0][0]);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &matrix[0][0], sizeof(glm::mat4));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &matrix[0][0],
                          sizeof(glm::mat4));
    }
#elif defined(METAL)
    updateMetalUniform(this, name, &matrix[0][0], sizeof(glm::mat4), true);
#endif
}

void Pipeline::setUniform3f(const std::string &name, float v0, float v1,
                            float v2) {
#ifdef OPENGL
    glUniform3f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    float data[3] = {v0, v1, v2};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#elif defined(METAL)
    float data[3] = {v0, v1, v2};
    updateMetalUniform(this, name, data, sizeof(data), true);
#endif
}

void Pipeline::setUniform1i(const std::string &name, int v0) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &v0, sizeof(int));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &v0,
                          sizeof(int));
    }
#elif defined(METAL)
    updateMetalUniform(this, name, &v0, sizeof(int), true);
#endif
}

void Pipeline::setUniformBool(const std::string &name, bool value) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()),
        (int)value);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    int intValue = value ? 1 : 0;
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &intValue, sizeof(int));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &intValue,
                          sizeof(int));
    }
#elif defined(METAL)
    int intValue = value ? 1 : 0;
    updateMetalUniform(this, name, &intValue, sizeof(int), true);
#endif
}

void Pipeline::setUniform4f(const std::string &name, float v0, float v1,
                            float v2, float v3) {
#ifdef OPENGL
    glUniform4f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2, v3);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    float data[4] = {v0, v1, v2, v3};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#elif defined(METAL)
    float data[4] = {v0, v1, v2, v3};
    updateMetalUniform(this, name, data, sizeof(data), true);
#endif
}

void Pipeline::setUniform2f(const std::string &name, float v0, float v1) {
#ifdef OPENGL
    glUniform2f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }
    float data[2] = {v0, v1};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#elif defined(METAL)
    float data[2] = {v0, v1};
    updateMetalUniform(this, name, data, sizeof(data), true);
#endif
}

void Pipeline::bindBufferData(const std::string &name, const void *data,
                              size_t size) {
    if (!shaderProgram || !data || size == 0) {
        return;
    }

#ifdef OPENGL
    (void)name;
    (void)data;
    (void)size;
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        logMissingUniformOnce(name);
        return;
    }

    if (!info->isBuffer) {
        return;
    }

    updateUniformData(info->set, info->binding, 0, data, size);
#elif defined(METAL)
    updateMetalUniform(this, name, data, size, false);
#endif
}

#ifdef VULKAN
Pipeline::UniformBufferAllocation &
Pipeline::getOrCreateUniformBuffer(uint32_t set, uint32_t binding,
                                   VkDeviceSize size) {

    const DescriptorBindingInfoEntry *bindingInfo =
        getDescriptorBindingInfo(set, binding);
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (bindingInfo) {
        descriptorType = bindingInfo->type;
    }
    VkBufferUsageFlags usage =
        descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            : VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    uint64_t key = makeBindingKey(set, binding);
    auto it = uniformBuffers.find(key);

    if (it != uniformBuffers.end()) {
        if (it->second.descriptorType == descriptorType &&
            it->second.size >= size && it->second.buffer != VK_NULL_HANDLE &&
            it->second.memory != VK_NULL_HANDLE &&
            it->second.mappedData != nullptr) {
            return it->second;
        }
        if (it->second.mappedData != nullptr &&
            it->second.memory != VK_NULL_HANDLE) {
            vkUnmapMemory(Device::globalDevice, it->second.memory);
        }
        if (it->second.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(Device::globalDevice, it->second.buffer, nullptr);
        }
        if (it->second.memory != VK_NULL_HANDLE) {
            vkFreeMemory(Device::globalDevice, it->second.memory, nullptr);
        }
        uniformBuffers.erase(it);
    }

    UniformBufferAllocation alloc{};
    alloc.size = size;
    alloc.descriptorType = descriptorType;
    alloc.buffer = VK_NULL_HANDLE;
    alloc.memory = VK_NULL_HANDLE;
    alloc.mappedData = nullptr;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device::globalDevice, &bufferInfo, nullptr,
                       &alloc.buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create uniform buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device::globalDevice, alloc.buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Device::globalInstance->findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(Device::globalDevice, &allocInfo, nullptr,
                         &alloc.memory) != VK_SUCCESS) {
        vkDestroyBuffer(Device::globalDevice, alloc.buffer, nullptr);
        throw std::runtime_error("Failed to allocate uniform buffer memory");
    }

    vkBindBufferMemory(Device::globalDevice, alloc.buffer, alloc.memory, 0);

    VkResult mapResult = vkMapMemory(Device::globalDevice, alloc.memory, 0,
                                     size, 0, &alloc.mappedData);
    if (mapResult != VK_SUCCESS || alloc.mappedData == nullptr) {
        vkDestroyBuffer(Device::globalDevice, alloc.buffer, nullptr);
        vkFreeMemory(Device::globalDevice, alloc.memory, nullptr);
        throw std::runtime_error("Failed to map uniform buffer memory");
    }

    memset(alloc.mappedData, 0, size);

    uniformBuffers[key] = alloc;
    return uniformBuffers[key];
}

const Pipeline::DescriptorBindingInfoEntry *
Pipeline::getDescriptorBindingInfo(uint32_t set, uint32_t binding) const {
    auto setIt = descriptorBindingInfo.find(set);
    if (setIt == descriptorBindingInfo.end()) {
        return nullptr;
    }
    auto bindingIt = setIt->second.find(binding);
    if (bindingIt == setIt->second.end()) {
        return nullptr;
    }
    return &bindingIt->second;
}

void Pipeline::updateUniformData(uint32_t set, uint32_t binding,
                                 uint32_t offset, const void *data,
                                 size_t size) {
    VkDeviceSize blockSize = 256;

    if (this->shaderProgram) {
        for (const auto &pair : this->shaderProgram->uniformBindings) {
            const UniformBindingInfo &info = pair.second;
            if (info.set == set && info.binding == binding && info.isBuffer &&
                info.size > 0) {
                if (info.offset == 0) {
                    blockSize = info.size;
                    break;
                }
            }
        }
    }

    VkDeviceSize requiredSize = offset + size;
    blockSize = std::max(blockSize, requiredSize);

    UniformBufferAllocation &alloc =
        getOrCreateUniformBuffer(set, binding, blockSize);

    if (alloc.mappedData == nullptr) {
        return;
    }

    char *dst = static_cast<char *>(alloc.mappedData) + offset;
    memcpy(dst, data, size);

    bindUniformBufferDescriptor(set, binding);
}

void Pipeline::buildDescriptorSets() {}

void Pipeline::resetDescriptorSets() {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(Device::globalDevice, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    descriptorSets.clear();
}

void Pipeline::ensureDescriptorResources() {
    if (descriptorBindingInfo.empty()) {
        return;
    }

    if (descriptorPool != VK_NULL_HANDLE && !descriptorSets.empty()) {
        bool allSetsValid = true;
        for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
            if (descriptorSetLayouts[i] != VK_NULL_HANDLE &&
                (i >= descriptorSets.size() ||
                 descriptorSets[i] == VK_NULL_HANDLE)) {
                allSetsValid = false;
                break;
            }
        }
        if (allSetsValid) {
            return;
        }
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        return;
    }

    std::unordered_map<VkDescriptorType, uint32_t> typeCounts;
    uint32_t setCount = 0;
    for (const auto &setPair : descriptorBindingInfo) {
        uint32_t setIdx = setPair.first;
        if (setIdx >= descriptorSetLayouts.size() ||
            descriptorSetLayouts[setIdx] == VK_NULL_HANDLE) {
            continue;
        }
        setCount++;
        for (const auto &bindingPair : setPair.second) {
            typeCounts[bindingPair.second.type] += bindingPair.second.count;
        }
    }

    if (setCount == 0 || typeCounts.empty()) {
        return;
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(typeCounts.size());
    for (const auto &pair : typeCounts) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = pair.first;
        poolSize.descriptorCount = pair.second;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = setCount;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    if (vkCreateDescriptorPool(Device::globalDevice, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    descriptorSets.assign(descriptorSetLayouts.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
        if (descriptorSetLayouts[i] == VK_NULL_HANDLE) {
            continue;
        }

        VkDescriptorSetLayout layout = descriptorSetLayouts[i];
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet set = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(Device::globalDevice, &allocInfo, &set) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }
        descriptorSets[i] = set;
    }

    auto dummyTex = getDummyTexture();
    auto dummyCubeTex = getDummyCubemap();
    for (const auto &setPair : descriptorBindingInfo) {
        uint32_t setIndex = setPair.first;
        if (setIndex >= descriptorSets.size() ||
            descriptorSets[setIndex] == VK_NULL_HANDLE) {
            continue;
        }
        for (const auto &bindingPair : setPair.second) {
            if (bindingPair.second.isBuffer) {
                bindUniformBufferDescriptor(setIndex, bindingPair.first);
            } else if (bindingPair.second.isSampler) {
                if (bindingPair.second.isCubemap) {
                    bindSamplerDescriptor(setIndex, bindingPair.first,
                                          dummyCubeTex);
                } else {
                    bindSamplerDescriptor(setIndex, bindingPair.first,
                                          dummyTex);
                }
            }
        }
    }
}

std::shared_ptr<Texture> Pipeline::getDummyTexture() {
    static std::shared_ptr<Texture> dummy = nullptr;
    if (!dummy) {
        uint8_t white[4] = {255, 255, 255, 255};
        dummy = Texture::create(TextureType::Texture2D, TextureFormat::Rgba8, 1,
                                1, TextureDataFormat::Rgba, white, 1);
    }
    return dummy;
}

std::shared_ptr<Texture> Pipeline::getDummyCubemap() {
    static std::shared_ptr<Texture> dummyCube = nullptr;
    if (!dummyCube) {
        dummyCube =
            Texture::create(TextureType::TextureCubeMap, TextureFormat::Rgba8,
                            1, 1, TextureDataFormat::Rgba, nullptr, 1);
    }
    return dummyCube;
}

void Pipeline::bindSamplerDescriptor(uint32_t set, uint32_t binding,
                                     std::shared_ptr<Texture> texture) {
    if (!texture) {
        return;
    }

    ensureDescriptorResources();

    if (set >= descriptorSets.size() || descriptorSets[set] == VK_NULL_HANDLE) {
        return;
    }

    const auto *info = getDescriptorBindingInfo(set, binding);
    if (info == nullptr || !info->isSampler) {
        return;
    }

    VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (texture->currentLayout != desiredLayout &&
        texture->currentLayout != VK_IMAGE_LAYOUT_GENERAL) {
        VkFormat vkFormat = opalTextureFormatToVulkanFormat(texture->format);
        bool isDepth = (texture->format == TextureFormat::Depth24Stencil8 ||
                        texture->format == TextureFormat::DepthComponent24 ||
                        texture->format == TextureFormat::Depth32F);

        if (!isDepth || texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            uint32_t layerCount =
                (texture->type == TextureType::TextureCubeMap) ? 6 : 1;
            Framebuffer::transitionImageLayout(texture->vkImage, vkFormat,
                                               texture->currentLayout,
                                               desiredLayout, layerCount);
            texture->currentLayout = desiredLayout;
        }
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture->vkSampler;
    imageInfo.imageView = texture->vkImageView;
    imageInfo.imageLayout =
        (texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : texture->currentLayout;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[set];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(Device::globalDevice, 1, &write, 0, nullptr);
}

void Pipeline::bindUniformBufferDescriptor(uint32_t set, uint32_t binding) {
    if (descriptorBindingInfo.empty()) {
        return;
    }

    ensureDescriptorResources();

    if (set >= descriptorSets.size() || descriptorSets[set] == VK_NULL_HANDLE) {
        return;
    }

    const auto *info = getDescriptorBindingInfo(set, binding);
    if (info == nullptr) {
        return;
    }
    if (!info->isBuffer) {
        return;
    }

    VkDeviceSize minSize = info->minBufferSize > 0 ? info->minBufferSize : 256;
    UniformBufferAllocation &alloc =
        getOrCreateUniformBuffer(set, binding, minSize);

    if (alloc.buffer == VK_NULL_HANDLE || alloc.memory == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = alloc.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = alloc.size;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[set];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = info->type;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(Device::globalDevice, 1, &write, 0, nullptr);
}

void Pipeline::bindDescriptorSets(VkCommandBuffer commandBuffer) {
    if (descriptorSetLayouts.empty()) {
        return;
    }

    ensureDescriptorResources();

    uint32_t currentStart = UINT32_MAX;
    std::vector<VkDescriptorSet> run;

    for (uint32_t i = 0; i < descriptorSets.size(); ++i) {
        bool setValid = i < descriptorSetLayouts.size() &&
                        descriptorSetLayouts[i] != VK_NULL_HANDLE &&
                        descriptorSets[i] != VK_NULL_HANDLE;

        if (!setValid) {
            if (!run.empty() && currentStart != UINT32_MAX) {
                vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, currentStart,
                    static_cast<uint32_t>(run.size()), run.data(), 0, nullptr);
                run.clear();
                currentStart = UINT32_MAX;
            }
            continue;
        }

        if (currentStart == UINT32_MAX) {
            currentStart = i;
        }
        run.push_back(descriptorSets[i]);
    }

    if (!run.empty() && currentStart != UINT32_MAX) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, currentStart,
                                static_cast<uint32_t>(run.size()), run.data(),
                                0, nullptr);
    }
}

void Pipeline::updatePushConstant(uint32_t offset, const void *data,
                                  size_t size) {
    uint32_t requiredSize = offset + static_cast<uint32_t>(size);
    if (pushConstantData.size() < requiredSize) {
        pushConstantData.resize(requiredSize, 0);
        pushConstantSize = requiredSize;
    }

    memcpy(pushConstantData.data() + offset, data, size);
    pushConstantsDirty = true;
}

void Pipeline::flushPushConstants(VkCommandBuffer commandBuffer) {
    if (pushConstantSize == 0) {
        return;
    }

    vkCmdPushConstants(commandBuffer, pipelineLayout, pushConstantStages, 0,
                       pushConstantSize, pushConstantData.data());

    pushConstantsDirty = false;
}
#endif

} // namespace opal
