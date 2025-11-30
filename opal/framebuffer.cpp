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

std::shared_ptr<RenderPass> RenderPass::create() {
    return std::make_shared<RenderPass>();
}

void RenderPass::setFramebuffer(std::shared_ptr<Framebuffer> framebuffer) {
    this->framebuffer = framebuffer;
}

std::shared_ptr<Framebuffer> Framebuffer::create(int width, int height) {
    auto framebuffer = std::make_shared<Framebuffer>();
    framebuffer->width = width;
    framebuffer->height = height;

#ifdef OPENGL
    glGenFramebuffers(1, &framebuffer->framebufferID);
#endif

    return framebuffer;
}

std::shared_ptr<Framebuffer> Framebuffer::create() {
    auto framebuffer = std::make_shared<Framebuffer>();
    framebuffer->width = 0;
    framebuffer->height = 0;

#ifdef OPENGL
    glGenFramebuffers(1, &framebuffer->framebufferID);
#endif

    return framebuffer;
}

void Framebuffer::attachTexture(std::shared_ptr<Texture> texture,
                                int attachmentIndex) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum attachmentType =
        GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachmentIndex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, texture->glType,
                           texture->textureID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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
    attachments.push_back(attachment);
#endif
}

void Framebuffer::attachCubemap(std::shared_ptr<Texture> texture,
                                Attachment::Type attachmentType) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum glAttachmentType;
    switch (attachmentType) {
    case Attachment::Type::Depth:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Color:
        glAttachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    default:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    }
    // Use glFramebufferTexture for cubemaps (attaches all 6 faces)
    glFramebufferTexture(GL_FRAMEBUFFER, glAttachmentType, texture->textureID,
                         0);

    Attachment att;
    att.type = attachmentType;
    att.texture = texture;
    attachments.push_back(att);
#endif
}

void Framebuffer::attachCubemapFace(std::shared_ptr<Texture> texture, int face,
                                    Attachment::Type attachmentType) {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLenum glAttachmentType;
    switch (attachmentType) {
    case Attachment::Type::Depth:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    case Attachment::Type::Color:
        glAttachmentType =
            GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(attachments.size());
        break;
    default:
        glAttachmentType = GL_DEPTH_ATTACHMENT;
        break;
    }
    // Attach a specific cubemap face (0-5: +X, -X, +Y, -Y, +Z, -Z)
    glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                           texture->textureID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Framebuffer::disableColorBuffer() {
#ifdef OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    colorBufferDisabled = true;
#endif
}

void Framebuffer::setViewport() {
#ifdef OPENGL
    glViewport(0, 0, width, height);
#endif
}

void Framebuffer::setViewport(int x, int y, int viewWidth, int viewHeight) {
#ifdef OPENGL
    glViewport(x, y, viewWidth, viewHeight);
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
    if (framebufferID == 0) {
        // Default framebuffer - draw to back buffer
        glDrawBuffer(GL_BACK);
    } else if (colorBufferDisabled) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else if (attachments.size() > 0) {
        std::vector<GLenum> drawBuffers;
        for (size_t i = 0; i < attachments.size(); ++i) {
            if (attachments[i].type == Attachment::Type::Color) {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 +
                                      static_cast<GLenum>(drawBuffers.size()));
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

void Framebuffer::setDrawBuffers(int attachmentCount) {
#ifdef OPENGL
    if (attachmentCount <= 0) {
        glDrawBuffer(GL_NONE);
        return;
    }
    std::vector<GLenum> drawBuffers;
    drawBuffers.reserve(attachmentCount);
    for (int i = 0; i < attachmentCount; ++i) {
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glDrawBuffers(attachmentCount, drawBuffers.data());
#endif
}

std::shared_ptr<ResolveAction>
ResolveAction::create(std::shared_ptr<Framebuffer> source,
                      std::shared_ptr<Framebuffer> destination) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = -1;
    action->resolveColor = true;
    action->resolveDepth = true;
    return action;
}

std::shared_ptr<ResolveAction> ResolveAction::createForColorAttachment(
    std::shared_ptr<Framebuffer> source,
    std::shared_ptr<Framebuffer> destination, int colorAttachmentIndex) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = colorAttachmentIndex;
    action->resolveColor = true;
    action->resolveDepth = false;
    return action;
}

std::shared_ptr<ResolveAction>
ResolveAction::createForDepth(std::shared_ptr<Framebuffer> source,
                              std::shared_ptr<Framebuffer> destination) {
    auto action = std::make_shared<ResolveAction>();
    action->source = source;
    action->destination = destination;
    action->colorAttachmentIndex = -1;
    action->resolveColor = false;
    action->resolveDepth = true;
    return action;
}

void CommandBuffer::performResolve(std::shared_ptr<ResolveAction> action) {
#ifdef OPENGL
    glBindFramebuffer(GL_READ_FRAMEBUFFER, action->source->framebufferID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, action->destination->framebufferID);

    GLbitfield mask = 0;

    if (action->resolveColor) {
        if (action->colorAttachmentIndex >= 0) {
            // Resolve specific color attachment
            glReadBuffer(GL_COLOR_ATTACHMENT0 + action->colorAttachmentIndex);
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + action->colorAttachmentIndex);
            mask |= GL_COLOR_BUFFER_BIT;
        } else {
            // Resolve all color attachments
            mask |= GL_COLOR_BUFFER_BIT;
        }
    }

    if (action->resolveDepth) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if (mask != 0) {
        glBlitFramebuffer(0, 0, action->source->width, action->source->height,
                          0, 0, action->destination->width,
                          action->destination->height, mask, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

} // namespace opal
