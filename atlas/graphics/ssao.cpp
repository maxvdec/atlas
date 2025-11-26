//
// ssao.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: SSAO Implementation for the Window Class
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/window.h"
#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/gtc/random.hpp>
#include <random>
#include <vector>
#include "opal/opal.h"

void Window::setupSSAO() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    this->ssaoKernel.clear();
    this->ssaoKernel.reserve(this->ssaoKernelSize);
    for (int i = 0; i < this->ssaoKernelSize; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) /
                      static_cast<float>(std::max(1, this->ssaoKernelSize));
        scale = 0.1f + 0.9f * (scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    this->ssaoNoise.clear();
    this->ssaoNoise.reserve(16);
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0,
                        randomFloats(generator) * 2.0 - 1.0, 0.0f);
        this->ssaoNoise.push_back(noise);
    }

    noiseTexture.texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Rgb16F, 4, 4,
        opal::TextureDataFormat::Rgb, this->ssaoNoise.data(), 1);
    noiseTexture.texture->setFilterMode(opal::TextureFilterMode::Nearest,
                                        opal::TextureFilterMode::Nearest);
    noiseTexture.texture->setWrapMode(opal::TextureAxis::S,
                                      opal::TextureWrapMode::Repeat);
    noiseTexture.texture->setWrapMode(opal::TextureAxis::T,
                                      opal::TextureWrapMode::Repeat);
    noiseTexture.id = noiseTexture.texture->textureID;
    noiseTexture.creationData.width = 4;
    noiseTexture.creationData.height = 4;
    noiseTexture.type = TextureType::SSAONoise;

    this->ssaoProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSAO);
    this->ssaoBlurProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSAOBlur);

    this->ssaoBuffer = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::SSAO));
    this->ssaoBlurBuffer = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::SSAOBlur));
    this->ssaoMapsDirty = true;
}

void Window::renderSSAO() {
    if (this->ssaoBuffer == nullptr || this->ssaoBlurBuffer == nullptr) {
        return;
    }

    this->ssaoUpdateCooldown =
        std::max(0.0f, this->ssaoUpdateCooldown - this->deltaTime);

    bool cameraMoved = false;
    if (this->camera != nullptr) {
        glm::vec3 currentPos = this->camera->position.toGlm();
        glm::vec3 currentDir = this->camera->getFrontVector().toGlm();

        if (!this->lastSSAOCameraPosition.has_value() ||
            !this->lastSSAOCameraDirection.has_value()) {
            cameraMoved = true;
        } else {
            glm::vec3 lastPos = this->lastSSAOCameraPosition->toGlm();
            glm::vec3 lastDir = this->lastSSAOCameraDirection->toGlm();
            if (glm::length(currentPos - lastPos) > 0.15f ||
                glm::length(currentDir - lastDir) > 0.015f) {
                cameraMoved = true;
            }
        }
    }

    if (cameraMoved) {
        this->ssaoMapsDirty = true;
    }

    if (!this->ssaoMapsDirty && this->ssaoUpdateCooldown > 0.0f) {
        return;
    }

    this->ssaoMapsDirty = false;
    this->ssaoUpdateCooldown = this->ssaoUpdateInterval;

    glBindFramebuffer(GL_FRAMEBUFFER, this->ssaoBuffer->fbo);
    glViewport(0, 0, this->ssaoBuffer->getWidth(),
               this->ssaoBuffer->getHeight());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    static std::shared_ptr<opal::DrawingState> ssaoState = nullptr;
    static std::shared_ptr<opal::Buffer> ssaoBuffer = nullptr;
    if (ssaoState == nullptr) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };
        ssaoBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        ssaoState = opal::DrawingState::create(ssaoBuffer);
        ssaoState->setBuffers(ssaoBuffer, nullptr);

        opal::VertexAttribute positionAttr{"ssaoPosition",
                                           opal::VertexAttributeType::Float,
                                           0,
                                           0,
                                           false,
                                           3,
                                           static_cast<uint>(5 * sizeof(float)),
                                           opal::VertexBindingInputRate::Vertex,
                                           0};
        opal::VertexAttribute uvAttr{"ssaoUV",
                                     opal::VertexAttributeType::Float,
                                     static_cast<uint>(3 * sizeof(float)),
                                     1,
                                     false,
                                     2,
                                     static_cast<uint>(5 * sizeof(float)),
                                     opal::VertexBindingInputRate::Vertex,
                                     0};

        std::vector<opal::VertexAttributeBinding> bindings = {
            {positionAttr, ssaoBuffer}, {uvAttr, ssaoBuffer}};
        ssaoState->configureAttributes(bindings);
    }

    // Get or create pipeline for SSAO
    static std::shared_ptr<opal::Pipeline> ssaoPipeline = nullptr;
    if (ssaoPipeline == nullptr) {
        ssaoPipeline = opal::Pipeline::create();
    }
    ssaoPipeline = this->ssaoProgram.requestPipeline(ssaoPipeline);
    ssaoPipeline->bind();

    ssaoPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id, 0);
    ssaoPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
    ssaoPipeline->bindTexture2D("texNoise", this->noiseTexture.id, 2);
    for (size_t i = 0; i < this->ssaoKernel.size(); ++i) {
        ssaoPipeline->setUniform3f("samples[" + std::to_string(i) + "]",
                                   ssaoKernel[i].x, ssaoKernel[i].y,
                                   ssaoKernel[i].z);
    }
    ssaoPipeline->setUniform1i("kernelSize",
                               static_cast<int>(this->ssaoKernel.size()));
    ssaoPipeline->setUniformMat4f("projection",
                                  this->calculateProjectionMatrix());
    ssaoPipeline->setUniformMat4f("view", getCamera()->calculateViewMatrix());
    glm::vec2 screenSize(this->ssaoBuffer->getWidth(),
                         this->ssaoBuffer->getHeight());
    glm::vec2 noiseSize(4.0f, 4.0f);
    ssaoPipeline->setUniform2f("noiseScale", screenSize.x / noiseSize.x,
                               screenSize.y / noiseSize.y);
    ssaoState->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ssaoState->unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, this->ssaoBlurBuffer->fbo);
    glViewport(0, 0, this->ssaoBlurBuffer->getWidth(),
               this->ssaoBlurBuffer->getHeight());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get or create pipeline for SSAO blur
    static std::shared_ptr<opal::Pipeline> ssaoBlurPipeline = nullptr;
    if (ssaoBlurPipeline == nullptr) {
        ssaoBlurPipeline = opal::Pipeline::create();
    }
    ssaoBlurPipeline = this->ssaoBlurProgram.requestPipeline(ssaoBlurPipeline);
    ssaoBlurPipeline->bind();

    ssaoBlurPipeline->bindTexture2D("inSSAO", this->ssaoBuffer->texture.id, 0);
    ssaoState->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ssaoState->unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (this->camera != nullptr) {
        this->lastSSAOCameraPosition = this->camera->position;
        this->lastSSAOCameraDirection = this->camera->getFrontVector();
    }
}