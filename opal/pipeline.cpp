//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for the core renderer
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

namespace opal {

std::shared_ptr<Pipeline> Pipeline::create() {
    auto pipeline = std::make_shared<Pipeline>();
    return pipeline;
}

void Pipeline::setShaderProgram(std::shared_ptr<ShaderProgram> program) {
#ifdef OPENGL
    this->shaderProgram = program;
#endif
}

void Pipeline::setVertexAttributes(
    const std::vector<VertexAttribute> &attributes,
    const VertexBinding &binding) {
#ifdef OPENGL
    this->vertexAttributes = attributes;
    this->vertexBinding = binding;
#endif
}

void Pipeline::setPrimitiveStyle(PrimitiveStyle style) {
#ifdef OPENGL
    this->primitiveStyle = style;
#endif
}

void Pipeline::setPatchVertices(int count) {
#ifdef OPENGL
    this->patchVertices = count;
#endif
}

void Pipeline::setViewport(int x, int y, int width, int height) {
#ifdef OPENGL
    this->viewportX = x;
    this->viewportY = y;
    this->viewportWidth = width;
    this->viewportHeight = height;
#endif
}

void Pipeline::setRasterizerMode(RasterizerMode mode) {
#ifdef OPENGL
    this->rasterizerMode = mode;
#endif
}

void Pipeline::setCullMode(CullMode mode) {
#ifdef OPENGL
    this->cullMode = mode;
#endif
}

void Pipeline::setFrontFace(FrontFace face) {
#ifdef OPENGL
    this->frontFace = face;
#endif
}

void Pipeline::enableDepthTest(bool enabled) {
#ifdef OPENGL
    this->depthTestEnabled = enabled;
#endif
}

void Pipeline::setDepthCompareOp(CompareOp op) {
#ifdef OPENGL
    this->depthCompareOp = op;
#endif
}

void Pipeline::enableDepthWrite(bool enabled) {
#ifdef OPENGL
    this->depthWriteEnabled = enabled;
#endif
}

void Pipeline::enableBlending(bool enabled) {
#ifdef OPENGL
    this->blendingEnabled = enabled;
#endif
}

void Pipeline::setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor) {
#ifdef OPENGL
    this->blendSrcFactor = srcFactor;
    this->blendDstFactor = dstFactor;
#endif
}

void Pipeline::setBlendEquation(BlendEquation equation) {
#ifdef OPENGL
    this->blendEquation = equation;
#endif
}

void Pipeline::enableMultisampling(bool enabled) {
#ifdef OPENGL
    this->multisamplingEnabled = enabled;
#endif
}

void Pipeline::enablePolygonOffset(bool enabled) {
#ifdef OPENGL
    this->polygonOffsetEnabled = enabled;
#endif
}

void Pipeline::setPolygonOffset(float factor, float units) {
#ifdef OPENGL
    this->polygonOffsetFactor = factor;
    this->polygonOffsetUnits = units;
#endif
}

void Pipeline::enableClipDistance(int index, bool enabled) {
#ifdef OPENGL
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
#endif
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
#endif
}

void Pipeline::setUniformMat4f(const std::string &name,
                               const glm::mat4 &matrix) {
#ifdef OPENGL
    glUniformMatrix4fv(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), 1,
        GL_FALSE, &matrix[0][0]);
#endif
}

void Pipeline::setUniform3f(const std::string &name, float v0, float v1,
                            float v2) {
#ifdef OPENGL
    glUniform3f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2);
#endif
}

void Pipeline::setUniform1i(const std::string &name, int v0) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0);
#endif
}

void Pipeline::setUniformBool(const std::string &name, bool value) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()),
        (int)value);
#endif
}

void Pipeline::setUniform4f(const std::string &name, float v0, float v1,
                            float v2, float v3) {
#ifdef OPENGL
    glUniform4f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2, v3);
#endif
}

void Pipeline::setUniform2f(const std::string &name, float v0, float v1) {
#ifdef OPENGL
    glUniform2f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1);
#endif
}

} // namespace opal
