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

        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture.id, 0);

        GLuint depthStencilRBO;
        glGenRenderbuffers(1, &depthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width,
                              size.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, depthStencilRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        }

        texture.creationData.width = size.width;
        texture.creationData.height = size.height;
        texture.type = TextureType::Color;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    } else if (type == RenderTargetType::Multisampled) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        GLuint msaaTex;
        glGenTextures(1, &msaaTex);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaTex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8,
                                size.width, size.height, GL_TRUE); // 4x MSAA
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, msaaTex, 0);

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width, size.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture.id, 0);

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
    if (type != RenderTargetType::Multisampled) {
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
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

    glUseProgram(obj->shaderProgram.programId);

    obj->shaderProgram.setUniform1i("Texture", 0);
    obj->shaderProgram.setUniform1i("TextureType",
                                    static_cast<int>(texture.type));
    obj->shaderProgram.setUniform1i("EffectCount", effects.size());
    for (int i = 0; i < effects.size(); i++) {
        std::string uniformName = "Effects[" + std::to_string(i) + "]";
        obj->shaderProgram.setUniform1i(uniformName,
                                        static_cast<int>(effects[i]->type));
        effects[i]->applyToProgram(obj->shaderProgram, i);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(obj->vao);
    if (!obj->indices.empty()) {
        glDrawElements(GL_TRIANGLES, obj->indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj->vertices.size());
    }
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}