//
// ssao.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: SSAO Implementation for the Window Class
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/window.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/gtc/random.hpp>
#include <random>
#include <vector>
#include "opal/opal.h"

void Window::setupSSAO() {
    int kernelSize = this->ssaoKernelSize;
#ifdef VULKAN
    if (kernelSize < 64) {
        kernelSize = 64;
        this->ssaoKernelSize = kernelSize;
    }
#endif
    atlas_log("Setting up SSAO (kernel size: " +
              std::to_string(kernelSize) + ")");
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    this->ssaoKernel.clear();
    this->ssaoKernel.reserve(kernelSize);
    for (int i = 0; i < kernelSize; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) /
                      static_cast<float>(std::max(1, kernelSize));
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

void Window::renderSSAO(std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (this->ssaoBuffer == nullptr || this->ssaoBlurBuffer == nullptr) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
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

    static std::shared_ptr<opal::DrawingState> ssaoState = nullptr;
    static std::shared_ptr<opal::Buffer> ssaoQuadBuffer = nullptr;
    const uint quadStride = static_cast<uint>(5 * sizeof(float));
    const opal::VertexAttribute positionAttr{
        .name = "ssaoPosition",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 3,
        .stride = quadStride,
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    const opal::VertexAttribute uvAttr{
        .name = "ssaoUV",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(3 * sizeof(float)),
        .location = 1,
        .normalized = false,
        .size = 2,
        .stride = quadStride,
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    const opal::VertexBinding quadBinding{quadStride,
                                          opal::VertexBindingInputRate::Vertex};

    // SSAO pass
    {
        auto ssaoRenderPass = opal::RenderPass::create();
        ssaoRenderPass->setFramebuffer(this->ssaoBuffer->getFramebuffer());
        commandBuffer->beginPass(ssaoRenderPass);
        this->ssaoBuffer->bind();
        commandBuffer->clearColor(1.0f, 1.0f, 1.0f, 1.0f);

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
            ssaoQuadBuffer =
                opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                     sizeof(quadVertices), quadVertices);
            ssaoState = opal::DrawingState::create(ssaoQuadBuffer);
            ssaoState->setBuffers(ssaoQuadBuffer, nullptr);

            std::vector<opal::VertexAttributeBinding> bindings = {
                {positionAttr, ssaoQuadBuffer}, {uvAttr, ssaoQuadBuffer}};
            ssaoState->configureAttributes(bindings);
        }

        static std::shared_ptr<opal::Pipeline> ssaoPipeline = nullptr;
        if (ssaoPipeline == nullptr) {
            ssaoPipeline = opal::Pipeline::create();
            ssaoPipeline->setShaderProgram(this->ssaoProgram.shader);
            std::vector<opal::VertexAttribute> quadAttributes = {positionAttr,
                                                                 uvAttr};
            ssaoPipeline->setVertexAttributes(quadAttributes, quadBinding);
            ssaoPipeline->build();
        }
        ssaoPipeline->setViewport(0, 0, this->ssaoBuffer->getWidth(),
                                  this->ssaoBuffer->getHeight());
        ssaoPipeline->bind();

        ssaoPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id,
                                    0);
        ssaoPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
        ssaoPipeline->bindTexture2D("texNoise", this->noiseTexture.id, 2);
        glm::vec2 screenSize(this->ssaoBuffer->getWidth(),
                             this->ssaoBuffer->getHeight());
        glm::vec2 noiseSize(4.0f, 4.0f);
#ifdef VULKAN
        struct alignas(16) VulkanSSAOParameters {
            glm::mat4 projection;
            glm::mat4 view;
            glm::vec2 noiseScale;
            glm::vec2 pad0;
        } params{};
        params.projection = this->calculateProjectionMatrix();
        params.view = getCamera()->calculateViewMatrix();
        params.noiseScale =
            glm::vec2(screenSize.x / noiseSize.x, screenSize.y / noiseSize.y);
        params.pad0 = glm::vec2(0.0f);

        struct alignas(16) VulkanSSAOSamples {
            glm::vec4 samples[64];
        } sampleData{};

        size_t sampleCount = std::min<size_t>(this->ssaoKernel.size(), 64);
        for (size_t i = 0; i < sampleCount; ++i) {
            sampleData.samples[i] = glm::vec4(this->ssaoKernel[i], 0.0f);
        }

        ssaoPipeline->bindBufferData("Paramters", &params, sizeof(params));
        ssaoPipeline->bindBufferData("Samples", &sampleData,
                                     sizeof(sampleData));
#else
        for (size_t i = 0; i < this->ssaoKernel.size(); ++i) {
            ssaoPipeline->setUniform3f("samples[" + std::to_string(i) + "]",
                                       ssaoKernel[i].x, ssaoKernel[i].y,
                                       ssaoKernel[i].z);
        }
        ssaoPipeline->setUniform1i("kernelSize",
                                   static_cast<int>(this->ssaoKernel.size()));
        ssaoPipeline->setUniformMat4f("projection",
                                      this->calculateProjectionMatrix());
        ssaoPipeline->setUniformMat4f("view",
                                      getCamera()->calculateViewMatrix());
        ssaoPipeline->setUniform2f("noiseScale", screenSize.x / noiseSize.x,
                                   screenSize.y / noiseSize.y);
#endif
        commandBuffer->bindPipeline(ssaoPipeline);
        commandBuffer->bindDrawingState(ssaoState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();
        this->ssaoBuffer->unbind();
        commandBuffer->endPass();
    }

    // SSAO blur pass
    {
        auto blurRenderPass = opal::RenderPass::create();
        blurRenderPass->setFramebuffer(this->ssaoBlurBuffer->getFramebuffer());
        commandBuffer->beginPass(blurRenderPass);
        this->ssaoBlurBuffer->bind();
        commandBuffer->clearColor(1.0f, 1.0f, 1.0f, 1.0f);

        static std::shared_ptr<opal::Pipeline> ssaoBlurPipeline = nullptr;
        if (ssaoBlurPipeline == nullptr) {
            ssaoBlurPipeline = opal::Pipeline::create();
            ssaoBlurPipeline->setShaderProgram(this->ssaoBlurProgram.shader);
            std::vector<opal::VertexAttribute> quadAttributes = {positionAttr,
                                                                 uvAttr};
            ssaoBlurPipeline->setVertexAttributes(quadAttributes, quadBinding);
            ssaoBlurPipeline->build();
        }
        ssaoBlurPipeline->setViewport(0, 0, this->ssaoBlurBuffer->getWidth(),
                                      this->ssaoBlurBuffer->getHeight());
        ssaoBlurPipeline->bind();

        ssaoBlurPipeline->bindTexture2D("inSSAO", this->ssaoBuffer->texture.id,
                                        0);
        commandBuffer->bindPipeline(ssaoBlurPipeline);
        commandBuffer->bindDrawingState(ssaoState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();
        this->ssaoBlurBuffer->unbind();
        commandBuffer->endPass();
    }

    if (this->camera != nullptr) {
        this->lastSSAOCameraPosition = this->camera->position;
        this->lastSSAOCameraDirection = this->camera->getFrontVector();
    }
}
