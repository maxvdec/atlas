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

        glGenTextures(1, &element.textureId);
        glBindTexture(GL_TEXTURE_2D, element.textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, element.intSize.x,
                     element.intSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

    if (vao == 0) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
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
    glUseProgram(downsampleProgram.programId);
    downsampleProgram.setUniform2f("srcResolution", srcViewportSizef.x,
                                   srcViewportSizef.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    downsampleProgram.setUniform1i("srcTexture", 0);

    for (size_t i = 0; i < elements.size(); i++) {
        const BloomElement &element = elements[i];
        glViewport(0, 0, element.size.x, element.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, element.textureId, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        downsampleProgram.setUniform2f("srcResolution", element.size.x,
                                       element.size.y);
        glBindTexture(GL_TEXTURE_2D, element.textureId);
    }
}

void BloomRenderTarget::renderUpsamples(float filterRadius) {
    glUseProgram(upsampleProgram.programId);
    upsampleProgram.setUniform1f("filterRadius", filterRadius);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = elements.size() - 1; i > 0; i--) {
        const BloomElement &element = elements[i];
        const BloomElement &nextElement = elements[i - 1];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, element.textureId);
        upsampleProgram.setUniform1i("srcTexture", 0);

        upsampleProgram.setUniform2f("srcResolution", element.size.x,
                                     element.size.y);

        glViewport(0, 0, nextElement.size.x, nextElement.size.y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, nextElement.textureId, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

void BloomRenderTarget::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}
