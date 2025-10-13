//
// ssao.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: SSAO Implementation for the Window Class
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/window.h"
#include <glm/gtc/random.hpp>
#include <random>

void Window::setupSSAO() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) / 64.0f;
        scale = 0.1f + 0.9f * (scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0,
                        randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }

    unsigned int noiseTextureID;
    glGenTextures(1, &noiseTextureID);
    glBindTexture(GL_TEXTURE_2D, noiseTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT,
                 &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    noiseTexture.id = noiseTextureID;
    noiseTexture.creationData.width = 4;
    noiseTexture.creationData.height = 4;
    noiseTexture.type = TextureType::SSAONoise;

    this->ssaoProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Fullscreen, AtlasFragmentShader::SSAO);

    this->ssaoBuffer = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::SSAO));
}

void Window::renderSSAO(RenderTarget *target) {
    glBindFramebuffer(GL_FRAMEBUFFER, this->ssaoBuffer->fbo);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    static Id ssaoVAO = 0;
    static Id ssaoVBO;
    if (ssaoVAO == 0) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };
        glGenVertexArrays(1, &ssaoVAO);
        glGenBuffers(1, &ssaoVBO);
        glBindVertexArray(ssaoVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ssaoVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
    }

    glUseProgram(this->ssaoProgram.programId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gPosition.id);
    this->ssaoProgram.setUniform1i("gPosition", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gNormal.id);
    this->ssaoProgram.setUniform1i("gNormal", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->noiseTexture.id);
    this->ssaoProgram.setUniform1i("texNoise", 2);
    for (unsigned int i = 0; i < 64; i++) {
        this->ssaoProgram.setUniform3f("samples[" + std::to_string(i) + "]",
                                       ssaoKernel[i].x, ssaoKernel[i].y,
                                       ssaoKernel[i].z);
    }
    this->ssaoProgram.setUniform1i("kernelSize", 64);
    this->ssaoProgram.setUniformMat4f("projection",
                                      this->calculateProjectionMatrix());
    glm::vec2 screenSize(target->texture.creationData.width,
                         target->texture.creationData.height);
    glm::vec2 noiseSize(4.0f, 4.0f);
    this->ssaoProgram.setUniform2f("noiseScale", screenSize.x / noiseSize.x,
                                   screenSize.y / noiseSize.y);
    glBindVertexArray(ssaoVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    this->ssaoBuffer->display(*this);
}