/*
 bloom.cpp
 As part of the Atlas project
 Created by Maxims Enterprise in 2024
 --------------------------------------------------
 Description:
 Copyright (c) 2025 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include <atlas/texture.h>
#include <climits>
#include <cstddef>
#include <glad/glad.h>
#include <vector>
#include "opal/opal.h"

void BloomRenderTarget::init(int width, int height, int chainLength) {
    if (initialized)
        return;
    initialized = true;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glm::vec2 mipSize((float)width, (float)height);
    glm::ivec2 mipIntSize((int)width, (int)height);
    if (width > INT_MAX || height > INT_MAX)
        throw std::runtime_error("Texture dimensions exceed maximum allowed");

    this->srcViewportSize = mipIntSize;
    this->srcViewportSizef = mipSize;

    for (int i = 0; i < chainLength; i++) {
        BloomElement element;
        mipSize *= 0.5f;
        mipIntSize /= 2;
        element.size = mipSize;
        element.intSize = mipIntSize;

        auto opalTexture = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgb16F,
            element.intSize.x, element.intSize.y);
        opalTexture->setFilterMode(opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);
        opalTexture->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
        opalTexture->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

        element.textureId = opalTexture->textureID;
        element.texture = opalTexture;

        this->elements.push_back(element);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           elements[0].textureId, 0);

    unsigned int attachments[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, attachments);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    downsampleProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Downsample);
    upsampleProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Upsample);

    if (quadState == nullptr) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };

        quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        quadState = opal::DrawingState::create(quadBuffer);
        quadState->setBuffers(quadBuffer, nullptr);

        opal::VertexAttribute positionAttr{"bloomPosition",
                                           opal::VertexAttributeType::Float,
                                           0,
                                           0,
                                           false,
                                           3,
                                           static_cast<uint>(5 * sizeof(float)),
                                           opal::VertexBindingInputRate::Vertex,
                                           0};
        opal::VertexAttribute uvAttr{"bloomUV",
                                     opal::VertexAttributeType::Float,
                                     static_cast<uint>(3 * sizeof(float)),
                                     1,
                                     false,
                                     2,
                                     static_cast<uint>(5 * sizeof(float)),
                                     opal::VertexBindingInputRate::Vertex,
                                     0};

        std::vector<opal::VertexAttributeBinding> bindings = {
            {positionAttr, quadBuffer}, {uvAttr, quadBuffer}};
        quadState->configureAttributes(bindings);
    }
}

void BloomRenderTarget::destroy() {
    for (size_t i = 0; i < elements.size(); i++) {
        glDeleteTextures(1, &elements[i].textureId);
        elements[i].textureId = 0;
    }
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;
    initialized = false;
}

const std::vector<BloomElement> &BloomRenderTarget::getElements() const {
    return elements;
}

void BloomRenderTarget::renderBloomTexture(unsigned int srcTexture,
                                           float filterRadius) {
    this->bindForWriting();

    this->renderDownsamples(srcTexture);
    this->renderUpsamples(filterRadius);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, srcViewportSize.x, srcViewportSize.y);
}

unsigned int BloomRenderTarget::getBloomTexture() {
    return elements[0].textureId;
}

void BloomRenderTarget::renderDownsamples(unsigned int srcTexture) {
    // Get or create pipeline for downsample
    static std::shared_ptr<opal::Pipeline> downsamplePipeline = nullptr;
    if (downsamplePipeline == nullptr) {
        downsamplePipeline = opal::Pipeline::create();
    }
    downsamplePipeline = downsampleProgram.requestPipeline(downsamplePipeline);
    downsamplePipeline->bind();

    downsamplePipeline->setUniform2f("srcResolution", srcViewportSizef.x,
                                     srcViewportSizef.y);
    downsamplePipeline->bindTexture2D("srcTexture", srcTexture, 0);

    for (size_t i = 0; i < elements.size(); i++) {
        const BloomElement &element = elements[i];
        glViewport(0, 0, element.size.x, element.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, element.textureId, 0);

        quadState->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadState->unbind();

        downsamplePipeline->setUniform2f("srcResolution", element.size.x,
                                         element.size.y);
        downsamplePipeline->bindTexture2D("srcTexture", element.textureId, 0);
    }
}

void BloomRenderTarget::renderUpsamples(float filterRadius) {
    // Get or create pipeline for upsample
    static std::shared_ptr<opal::Pipeline> upsamplePipeline = nullptr;
    if (upsamplePipeline == nullptr) {
        upsamplePipeline = opal::Pipeline::create();
    }
    upsamplePipeline = upsampleProgram.requestPipeline(upsamplePipeline);
    upsamplePipeline->bind();

    upsamplePipeline->setUniform1f("filterRadius", filterRadius);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = elements.size() - 1; i > 0; i--) {
        const BloomElement &element = elements[i];
        const BloomElement &nextElement = elements[i - 1];

        upsamplePipeline->bindTexture2D("srcTexture", element.textureId, 0);
        upsamplePipeline->setUniform2f("srcResolution", element.size.x,
                                       element.size.y);

        glViewport(0, 0, nextElement.size.x, nextElement.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, nextElement.textureId, 0);

        quadState->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadState->unbind();
    }

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

void BloomRenderTarget::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}
