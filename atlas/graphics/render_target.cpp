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
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"

RenderTarget RenderTarget::create(Window &window, RenderTargetType type) {
    Id fbo;
    Id rbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    Texture texture;

    if (type == RenderTargetType::Scene) {
        int width = static_cast<int>(window.getSize().x);
        int height = static_cast<int>(window.getSize().y);

        Id textureColorBuffer;
        glGenTextures(1, &textureColorBuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, textureColorBuffer, 0);

        texture.id = textureColorBuffer;
        texture.type = TextureType::Color;
        texture.creationData = TextureCreationData{width, height, 3};

        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                              height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, rbo);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            switch (status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                throw std::runtime_error(
                    "Framebuffer incomplete: Attachment is NOT complete");
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                throw std::runtime_error(
                    "Framebuffer incomplete: No image is attached to FBO");
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                throw std::runtime_error("Framebuffer incomplete: Draw buffer");
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                throw std::runtime_error("Framebuffer incomplete: Read buffer");
            case GL_FRAMEBUFFER_UNSUPPORTED:
                throw std::runtime_error("Framebuffer incomplete: Unsupported "
                                         "by FBO implementation");
            default:
                throw std::runtime_error(
                    "Framebuffer incomplete: Unknown error");
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    } else {
        throw std::runtime_error("Unsupported render target type");
    }

    RenderTarget renderTarget;
    renderTarget.fbo = fbo;
    renderTarget.rbo = rbo;
    renderTarget.texture = texture;
    renderTarget.type = type;

    return renderTarget;
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
        window.addPreferencedObject(this->object.get());
    } else {
        this->object->show();
    }
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
