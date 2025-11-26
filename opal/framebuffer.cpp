//
// framebuffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Framebuffer abstractions for Opal
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include <memory>

namespace opal {

std::shared_ptr<Framebuffer> Framebuffer::create(int width, int height) {
    auto framebuffer = std::make_shared<Framebuffer>();
    framebuffer->width = width;
    framebuffer->height = height;

#ifdef OPENGL
    glGenFramebuffers(1, &framebuffer->framebufferID);
#endif

    return framebuffer;
}

void Framebuffer::addAttachment(const Attachment &attachment) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum attachmentType;
    switch (attachment.type) {
    case Attachment::Type::Color:
        attachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    case Attachment::Type::Depth:
        attachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Stencil:
        attachmentType = GL_STENCIL_ATTACHMENT;
        break;
    case Attachment::Type::DepthStencil:
        attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
        break;
    default:
        throw std::runtime_error("Unknown attachment type");
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType,
                           attachment.texture->glType,
                           attachment.texture->textureID, 0);
#endif
}

bool Framebuffer::getStatus() const {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
#else
    return false;
#endif
}

void Framebuffer::bind() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    if (attachments.size() > 0) {
        std::vector<GLenum> drawBuffers;
        for (size_t i = 0; i < attachments.size(); ++i) {
            if (attachments[i].type == Attachment::Type::Color) {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 +
                                      static_cast<GLenum>(i));
            }
        }
        if (drawBuffers.empty()) {
            glDrawBuffer(GL_NONE);
        } else {
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()),
                          drawBuffers.data());
        }
    } else {
        glDrawBuffer(GL_NONE);
    }
#endif
}

void Framebuffer::unbind() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Framebuffer::bindForRead() {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferID);
#endif
}

void Framebuffer::bindForDraw() {
#ifdef OPENGL
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferID);
#endif
}

std::shared_ptr<ResolveAction>
ResolveAction::create(std::shared_ptr<Framebuffer> source,
                      std::shared_ptr<Framebuffer> destination) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    return action;
}

void CommandBuffer::performResolve(std::shared_ptr<ResolveAction> action) {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, action->source->framebufferID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, action->destination->framebufferID);
    glBlitFramebuffer(0, 0, action->source->width, action->source->height, 0, 0,
                      action->destination->width, action->destination->height,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#endif
}

} // namespace opal
