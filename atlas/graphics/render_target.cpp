/*
 render_target.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions and definitions for render targets
 Copyright (c) 2025 maxvdec
*/

#include <glad/glad.h>
#include <iostream>
#include <vector>
#include "atlas/camera.h"
#include "atlas/core/shader.h"
#include "atlas/effect.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"

RenderTarget::RenderTarget(Window &window, RenderTargetType type,
                           int resolution) {
    Size2d size = window.getSize();
    this->type = type;

    if (type == RenderTargetType::Scene) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        unsigned int colorTextures[2];
        glGenTextures(2, colorTextures);
        for (unsigned int i = 0; i < 2; i++) {
            glBindTexture(GL_TEXTURE_2D, colorTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.width, size.height,
                         0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                   GL_TEXTURE_2D, colorTextures[i], 0);
        }

        unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0,
                                       GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);

        GLuint depthTexture;
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.width,
                     size.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, depthTexture, 0);
        this->depthTexture.id = depthTexture;
        this->depthTexture.creationData.width = size.width;
        this->depthTexture.creationData.height = size.height;
        this->depthTexture.type = TextureType::Depth;

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        }

        texture.creationData.width = size.width;
        texture.creationData.height = size.height;
        texture.type = TextureType::Color;
        texture.id = colorTextures[0];

        brightTexture.creationData.width = size.width;
        brightTexture.creationData.height = size.height;
        brightTexture.type = TextureType::Color;
        brightTexture.id = colorTextures[1];

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    } else if (type == RenderTargetType::Multisampled) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        GLuint msaaTex;
        glGenTextures(1, &msaaTex);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F,
                                size.width, size.height, GL_TRUE); // 4x MSAA
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, msaaTex, 0);

        GLenum msaaDrawBuf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &msaaDrawBuf);

        GLuint depthStencilRBO;
        glGenRenderbuffers(1, &depthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, size.width, size.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, depthStencilRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: MSAA Framebuffer is not complete!"
                      << std::endl;
        }

        glGenFramebuffers(1, &resolveFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);

        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.width, size.height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture.id, 0);

        GLenum resolveDrawBuf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &resolveDrawBuf);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Resolve Framebuffer is not complete!"
                      << std::endl;
        }

        texture.creationData.width = size.width;
        texture.creationData.height = size.height;
        texture.type = TextureType::Color;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else if (this->type == RenderTargetType::Shadow) {
        Id depthFBO;
        glGenFramebuffers(1, &depthFBO);

        int SHADOW_WIDTH = resolution, SHADOW_HEIGHT = resolution;

        Id depthMap;
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                     SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Texture texture;
        texture.id = depthMap;
        texture.creationData = {SHADOW_WIDTH, SHADOW_HEIGHT, 1};
        texture.type = TextureType::Depth;

        this->fbo = depthFBO;
        this->texture = texture;
        this->rbo = 0;
        this->resolveFbo = 0;
    } else if (this->type == RenderTargetType::CubeShadow) {
        Id depthFBO;
        glGenFramebuffers(1, &depthFBO);

        int SHADOW_WIDTH = resolution, SHADOW_HEIGHT = resolution;

        Id depthCubemap;
        glGenTextures(1, &depthCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                         GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                        GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap,
                             0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Texture texture;
        texture.id = depthCubemap;
        texture.creationData = {SHADOW_WIDTH, SHADOW_HEIGHT, 1};
        texture.type = TextureType::DepthCube;
        this->fbo = depthFBO;
        this->texture = texture;
        this->rbo = 0;
        this->resolveFbo = 0;
    } else if (this->type == RenderTargetType::GBuffer) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &gPosition.id);
        glBindTexture(GL_TEXTURE_2D, gPosition.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.width, size.height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, gPosition.id, 0);
        gPosition.creationData.width = size.width;
        gPosition.creationData.height = size.height;
        gPosition.type = TextureType::Color;

        glGenTextures(1, &gNormal.id);
        glBindTexture(GL_TEXTURE_2D, gNormal.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.width, size.height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                               GL_TEXTURE_2D, gNormal.id, 0);
        gNormal.creationData.width = size.width;
        gNormal.creationData.height = size.height;
        gNormal.type = TextureType::Color;

        glGenTextures(1, &gAlbedoSpec.id);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
                               GL_TEXTURE_2D, gAlbedoSpec.id, 0);
        gAlbedoSpec.creationData.width = size.width;
        gAlbedoSpec.creationData.height = size.height;
        gAlbedoSpec.type = TextureType::Color;

        glGenTextures(1, &gMaterial.id);
        glBindTexture(GL_TEXTURE_2D, gMaterial.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
                               GL_TEXTURE_2D, gMaterial.id, 0);
        gMaterial.creationData.width = size.width;
        gMaterial.creationData.height = size.height;
        gMaterial.type = TextureType::Color;

        glGenTextures(1, &depthTexture.id);
        glBindTexture(GL_TEXTURE_2D, depthTexture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.width,
                     size.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, depthTexture.id, 0);
        depthTexture.creationData.width = size.width;
        depthTexture.creationData.height = size.height;
        depthTexture.type = TextureType::Depth;

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: GBuffer Framebuffer is not complete!"
                      << std::endl;
        }

        unsigned int attachments[4] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, attachments);
    }
}

void RenderTarget::display(Window &window, float zindex) {
    if (object == nullptr) {
        CoreObject obj;
        std::vector<CoreVertex> vertices = {
            // positions        // texture coords
            {{1.0f, 1.0f, zindex}, Color::white(), {1.0f, 1.0f}}, // top right
            {{1.0f, -1.0f, zindex},
             Color::white(),
             {1.0f, 0.0f}}, // bottom right
            {{-1.0f, -1.0f, zindex},
             Color::white(),
             {0.0f, 0.0f}},                                       // bottom left
            {{-1.0f, 1.0f, zindex}, Color::white(), {0.0f, 1.0f}} // top left
        };
        VertexShader vertexShader =
            VertexShader::fromDefaultShader(AtlasVertexShader::Fullscreen);
        FragmentShader fragmentShader =
            FragmentShader::fromDefaultShader(AtlasFragmentShader::Fullscreen);

        obj.createAndAttachProgram(vertexShader, fragmentShader);

        std::vector<Index> indices = {0, 1, 3, 1, 2, 3};

        obj.attachTexture(this->texture);
        obj.attachVertices(vertices);
        obj.attachIndices(indices);
        obj.renderOnlyTexture();
        obj.show();
        obj.initialize();
        this->object = std::make_shared<CoreObject>(obj);
        window.addPreferencedObject(this);
    } else {
        this->object->show();
    }
}

void RenderTarget::resolve() {
    glBindTexture(GL_TEXTURE_2D, this->texture.id);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (type != RenderTargetType::Multisampled) {
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, texture.creationData.width,
                      texture.creationData.height, 0, 0,
                      texture.creationData.width, texture.creationData.height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::hide() {
    if (object != nullptr) {
        object->hide();
    } else {
        throw std::runtime_error("Render target object is null");
    }
}

void RenderTarget::show() {
    if (object != nullptr) {
        object->show();
    } else {
        throw std::runtime_error("Render target object is null");
    }
}

void RenderTarget::render(float dt) {
    if (!object || !object->isVisible) {
        return;
    }

    if (type == RenderTargetType::Multisampled) {
        resolve();
    }

    CoreObject *obj = this->object.get();

    for (int i = 0; i < 16; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(0);

    GLuint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&currentProgram);
    if (currentProgram != obj->shaderProgram.programId) {
        glUseProgram(obj->shaderProgram.programId);
    }

    Camera *camera = Window::mainWindow->camera;

    if (texture.type == TextureType::DepthCube) {
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);
        obj->shaderProgram.setUniform1i("cubeMap", 10);
        obj->shaderProgram.setUniform1i("isCubeMap", 1);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        obj->shaderProgram.setUniform1i("Texture", 0);
        obj->shaderProgram.setUniform1i("isCubeMap", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blurredTexture.id);
        obj->shaderProgram.setUniform1i("BrightTexture", 1);
        obj->shaderProgram.setUniform1i("hasBrightTexture",
                                        brightTexture.id != 0 ? 1 : 0);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthTexture.id);
        obj->shaderProgram.setUniform1i("DepthTexture", 2);
        if (!this->renderDepthOfView) {
            obj->shaderProgram.setUniform1i("hasDepthTexture", 0);
        } else {
            obj->shaderProgram.setUniform1i("hasDepthTexture",
                                            depthTexture.id != 0 ? 1 : 0);
        }

        obj->shaderProgram.setUniform1f("nearPlane", camera->nearClip);
        obj->shaderProgram.setUniform1f("farPlane", camera->farClip);
        obj->shaderProgram.setUniform1f("focusDepth", camera->focusDepth);
        obj->shaderProgram.setUniform1f("focusRange", camera->focusRange);

        int maxMipLevels = (int)std::floor(
            std::log2(std::max(Window::mainWindow->getSize().width,
                               Window::mainWindow->getSize().height)));

        obj->shaderProgram.setUniform1i("maxMipLevel", maxMipLevels);
    }

    obj->shaderProgram.setUniform1i("TextureType",
                                    static_cast<int>(texture.type));
    obj->shaderProgram.setUniform1i("EffectCount", effects.size());

    for (int i = 0; i < effects.size(); i++) {
        std::string uniformName = "Effects[" + std::to_string(i) + "]";
        obj->shaderProgram.setUniform1i(uniformName,
                                        static_cast<int>(effects[i]->type));
        effects[i]->applyToProgram(obj->shaderProgram, i);
    }

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(obj->vao);

    if (!obj->indices.empty()) {
        glDrawElements(GL_TRIANGLES, obj->indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj->vertices.size());
    }

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}
