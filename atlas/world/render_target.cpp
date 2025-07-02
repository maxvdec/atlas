/*
 render_target.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Render Target functions and implementations
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/texture.hpp"
#include "atlas/window.hpp"
#include <iostream>
#include <memory>

RenderTarget::RenderTarget(Size2d size, TextureType type) : size(size) {
    this->texture.size = size;
    this->texture.type = type;

    const int samples = 4;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLuint colorRBO;
    glGenRenderbuffers(1, &colorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8,
                                     size.width, size.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, colorRBO);

    GLuint depthStencilRBO;
    glGenRenderbuffers(1, &depthStencilRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, size.width, size.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, depthStencilRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Error: Multisampled Framebuffer is not complete!"
                  << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &resolveFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);

    this->texture = Texture();
    glGenTextures(1, &this->texture.ID);
    glBindTexture(GL_TEXTURE_2D, this->texture.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           this->texture.ID, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Error: Resolve Framebuffer is not complete!" << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Window::current_window->renderTargets.push_back(this);
}

void RenderTarget::renderToScreen() {
    if (!isOn) {
        std::cerr << "RenderTarget is not enabled!" << std::endl;
        return;
    }

    this->fullScreenObject =
        std::make_unique<CoreObject>(presentFullScreenTexture(this->texture));
    fullScreenObject->initCore();

    this->dispatcher = [](CoreObject *object, RenderTarget *target) {
        if (object == nullptr || !object->program.has_value()) {
            std::cerr << "Error: Attempting to render a null object."
                      << std::endl;
            return;
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->resolveFbo);
        glBlitFramebuffer(0, 0, target->size.width, target->size.height, 0, 0,
                          target->size.width, target->size.height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(object->program.value().ID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, object->textures[0].ID);

        GLint boundTex;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
        if (boundTex != static_cast<GLint>(object->textures[0].ID)) {
            std::cerr << "Texture binding failed!" << std::endl;
        }

        object->program.value().setInt("uTexture1", 0);
        object->program.value().setVec2(
            "uTexelSize",
            glm::vec2(1.0f / target->size.width, 1.0f / target->size.height));

        for (const auto &effect : target->effects) {
            switch (effect.type) {
            case EffectType::Inverse:
                object->program.value().setBool("uInverted", true);
                break;
            case EffectType::Grayscale:
                object->program.value().setBool("uGrayscale", true);
                break;
            case EffectType::Kernel:
                object->program.value().setBool("uKernel", true);
                object->program.value().setFloat("uKernelIntensity",
                                                 effect.intensity);
                break;
            case EffectType::Blur:
                object->program.value().setBool("uBlur", true);
                object->program.value().setFloat("uBlurIntensity",
                                                 effect.intensity);
                break;
            case EffectType::EdgeDetection:
                object->program.value().setBool("uEdgeDetection", true);
                object->program.value().setFloat("uKernelIntensity",
                                                 effect.intensity);
                break;
            default:
                std::cerr << "Unknown effect!" << std::endl;
                break;
            }
        }

        glBindVertexArray(object->attributes.VAO);
        if (object->attributes.EBO.has_value()) {
            glDrawElements(GL_TRIANGLES, object->attributes.elementCount,
                           GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0,
                         static_cast<GLsizei>(object->vertices.size()));
        }
    };

    this->isRendering = true;
}
