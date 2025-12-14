/*
 render_target.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions and definitions for render targets
 Copyright (c) 2025 maxvdec
*/

#include <cstddef>
#include <glad/glad.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include "atlas/camera.h"
#include "atlas/core/shader.h"
#include "atlas/effect.h" // IWYU pragma: keep
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "opal/opal.h"

RenderTarget::RenderTarget(Window &window, RenderTargetType type,
                           int resolution) {
    atlas_log("Creating render target (type: " +
              std::to_string(static_cast<int>(type)) + ")");
    GLFWwindow *glfwWindow = static_cast<GLFWwindow *>(window.windowRef);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);

    float targetScale = window.getRenderScale();
    if (type == RenderTargetType::SSAO || type == RenderTargetType::SSAOBlur) {
        targetScale = window.getSSAORenderScale();
    }
    targetScale = std::clamp(targetScale, 0.1f, 1.0f);

    int scaledWidth = std::max(1, static_cast<int>(fbWidth * targetScale));
    int scaledHeight = std::max(1, static_cast<int>(fbHeight * targetScale));
    Size2d size;
    size = {.width = static_cast<float>(scaledWidth),
            .height = static_cast<float>(scaledHeight)};
    const auto width = static_cast<GLsizei>(scaledWidth);
    const auto height = static_cast<GLsizei>(scaledHeight);
    this->type = type;

    if (type == RenderTargetType::Scene) {
        fb = opal::Framebuffer::create(width, height);
        std::vector<std::shared_ptr<opal::Texture>> colorTextures;
        for (unsigned int i = 0; i < 2; i++) {
            auto texture = opal::Texture::create(
                opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F,
                width, height, opal::TextureDataFormat::Rgba, nullptr, 1);
            texture->setFilterMode(opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);
            texture->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
            texture->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);
            opal::Attachment attachment;
            attachment.texture = texture;
            attachment.type = opal::Attachment::Type::Color;
            colorTextures.push_back(texture);
            fb->addAttachment(attachment);
        }

        auto depthTexture = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::DepthComponent24,
            width, height, opal::TextureDataFormat::DepthComponent, nullptr, 1);
        opal::Attachment depthAttachment;
        depthAttachment.texture = depthTexture;
        depthAttachment.type = opal::Attachment::Type::Depth;
        fb->addAttachment(depthAttachment);

        this->depthTexture.texture = depthTexture;
        this->depthTexture.creationData.width = scaledWidth;
        this->depthTexture.creationData.height = scaledHeight;
        this->depthTexture.type = TextureType::Depth;

        if (fb->getStatus() == false) {
            atlas_error("Framebuffer is not complete for Scene render target");
            std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        }

        texture.creationData.width = scaledWidth;
        texture.creationData.height = scaledHeight;
        texture.type = TextureType::Color;
        texture.texture = colorTextures[0];

        brightTexture.creationData.width = scaledWidth;
        brightTexture.creationData.height = scaledHeight;
        brightTexture.type = TextureType::Color;
        brightTexture.texture = colorTextures[1];

        fb->unbind();
    } else if (type == RenderTargetType::Multisampled) {
        const int samples = 4;

        fb = opal::Framebuffer::create(width, height);

        auto msColor0 = opal::Texture::createMultisampled(
            opal::TextureFormat::Rgba16F, width, height, samples);
        auto msColor1 = opal::Texture::createMultisampled(
            opal::TextureFormat::Rgba16F, width, height, samples);

        opal::Attachment colorAttachment0;
        colorAttachment0.texture = msColor0;
        colorAttachment0.type = opal::Attachment::Type::Color;
        fb->addAttachment(colorAttachment0);

        opal::Attachment colorAttachment1;
        colorAttachment1.texture = msColor1;
        colorAttachment1.type = opal::Attachment::Type::Color;
        fb->addAttachment(colorAttachment1);

        auto msDepth = opal::Texture::createMultisampled(
            opal::TextureFormat::DepthComponent24, width, height, samples);

        opal::Attachment depthAttachment;
        depthAttachment.texture = msDepth;
        depthAttachment.type = opal::Attachment::Type::Depth;
        fb->addAttachment(depthAttachment);

        if (!fb->getStatus()) {
            std::cerr << "Error: Multisampled framebuffer is not complete!"
                      << std::endl;
        }

        this->msTexture.texture = msColor0;
        this->msTexture.creationData.width = scaledWidth;
        this->msTexture.creationData.height = scaledHeight;
        this->msTexture.type = TextureType::Color;

        this->msBrightTexture.texture = msColor1;
        this->msBrightTexture.creationData.width = scaledWidth;
        this->msBrightTexture.creationData.height = scaledHeight;
        this->msBrightTexture.type = TextureType::Color;

        this->msDepthTexture.texture = msDepth;
        this->msDepthTexture.creationData.width = scaledWidth;
        this->msDepthTexture.creationData.height = scaledHeight;
        this->msDepthTexture.type = TextureType::Depth;

        resolveFb = opal::Framebuffer::create(width, height);

        auto resolvedColor0 = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        resolvedColor0->setFilterMode(opal::TextureFilterMode::Linear,
                                      opal::TextureFilterMode::Linear);
        resolvedColor0->setWrapMode(opal::TextureAxis::S,
                                    opal::TextureWrapMode::ClampToEdge);
        resolvedColor0->setWrapMode(opal::TextureAxis::T,
                                    opal::TextureWrapMode::ClampToEdge);

        auto resolvedColor1 = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        resolvedColor1->setFilterMode(opal::TextureFilterMode::Linear,
                                      opal::TextureFilterMode::Linear);
        resolvedColor1->setWrapMode(opal::TextureAxis::S,
                                    opal::TextureWrapMode::ClampToEdge);
        resolvedColor1->setWrapMode(opal::TextureAxis::T,
                                    opal::TextureWrapMode::ClampToEdge);

        opal::Attachment resolveColorAttachment0;
        resolveColorAttachment0.texture = resolvedColor0;
        resolveColorAttachment0.type = opal::Attachment::Type::Color;
        resolveFb->addAttachment(resolveColorAttachment0);

        opal::Attachment resolveColorAttachment1;
        resolveColorAttachment1.texture = resolvedColor1;
        resolveColorAttachment1.type = opal::Attachment::Type::Color;
        resolveFb->addAttachment(resolveColorAttachment1);

        auto resolvedDepth = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::DepthComponent24,
            width, height, opal::TextureDataFormat::DepthComponent, nullptr, 1);
        resolvedDepth->setFilterMode(opal::TextureFilterMode::Linear,
                                     opal::TextureFilterMode::Linear);

        opal::Attachment resolveDepthAttachment;
        resolveDepthAttachment.texture = resolvedDepth;
        resolveDepthAttachment.type = opal::Attachment::Type::Depth;
        resolveFb->addAttachment(resolveDepthAttachment);

        if (!resolveFb->getStatus()) {
            std::cerr << "Error: Resolve framebuffer is not complete!"
                      << std::endl;
        }

        texture.texture = resolvedColor0;
        texture.creationData.width = scaledWidth;
        texture.creationData.height = scaledHeight;
        texture.type = TextureType::Color;

        brightTexture.texture = resolvedColor1;
        brightTexture.creationData.width = scaledWidth;
        brightTexture.creationData.height = scaledHeight;
        brightTexture.type = TextureType::Color;

        this->depthTexture.texture = resolvedDepth;
        this->depthTexture.creationData.width = scaledWidth;
        this->depthTexture.creationData.height = scaledHeight;
        this->depthTexture.type = TextureType::Depth;

        resolveFb->unbind();
    } else if (this->type == RenderTargetType::Shadow) {
        int SHADOW_WIDTH = resolution, SHADOW_HEIGHT = resolution;

        fb = opal::Framebuffer::create(SHADOW_WIDTH, SHADOW_HEIGHT);

        auto depthMap = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::DepthComponent24,
            SHADOW_WIDTH, SHADOW_HEIGHT,
            opal::TextureDataFormat::DepthComponent, nullptr, 1);
        depthMap->setFilterMode(opal::TextureFilterMode::Nearest,
                                opal::TextureFilterMode::Nearest);
        depthMap->setWrapMode(opal::TextureAxis::S,
                              opal::TextureWrapMode::Repeat);
        depthMap->setWrapMode(opal::TextureAxis::T,
                              opal::TextureWrapMode::Repeat);

        opal::Attachment depthAttachment;
        depthAttachment.texture = depthMap;
        depthAttachment.type = opal::Attachment::Type::Depth;
        fb->addAttachment(depthAttachment);
        fb->disableColorBuffer();

        if (!fb->getStatus()) {
            std::cerr << "Error: Shadow framebuffer is not complete!"
                      << std::endl;
        }

        texture.texture = depthMap;
        texture.creationData = {SHADOW_WIDTH, SHADOW_HEIGHT, 1};
        texture.type = TextureType::Depth;

        fb->unbind();
    } else if (this->type == RenderTargetType::CubeShadow) {
        int SHADOW_WIDTH = resolution, SHADOW_HEIGHT = resolution;

        fb = opal::Framebuffer::create(SHADOW_WIDTH, SHADOW_HEIGHT);

        auto depthCubemap = opal::Texture::createDepthCubemap(
            opal::TextureFormat::DepthComponent24, SHADOW_WIDTH);

        fb->attachCubemap(depthCubemap, opal::Attachment::Type::Depth);
        fb->disableColorBuffer();

        if (!fb->getStatus()) {
            std::cerr << "Error: CubeShadow framebuffer is not complete!"
                      << std::endl;
        }

        texture.texture = depthCubemap;
        texture.creationData = {SHADOW_WIDTH, SHADOW_HEIGHT, 1};
        texture.type = TextureType::DepthCube;

        fb->unbind();
    } else if (this->type == RenderTargetType::GBuffer) {
        fb = opal::Framebuffer::create(width, height);

        auto positionTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        positionTex->setFilterMode(opal::TextureFilterMode::Nearest,
                                   opal::TextureFilterMode::Nearest);
        positionTex->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
        positionTex->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

        opal::Attachment positionAttachment;
        positionAttachment.texture = positionTex;
        positionAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(positionAttachment);

        gPosition.texture = positionTex;
        gPosition.creationData.width = scaledWidth;
        gPosition.creationData.height = scaledHeight;
        gPosition.type = TextureType::Color;

        auto normalTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        normalTex->setFilterMode(opal::TextureFilterMode::Nearest,
                                 opal::TextureFilterMode::Nearest);
        normalTex->setWrapMode(opal::TextureAxis::S,
                               opal::TextureWrapMode::ClampToEdge);
        normalTex->setWrapMode(opal::TextureAxis::T,
                               opal::TextureWrapMode::ClampToEdge);

        opal::Attachment normalAttachment;
        normalAttachment.texture = normalTex;
        normalAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(normalAttachment);

        gNormal.texture = normalTex;
        gNormal.creationData.width = scaledWidth;
        gNormal.creationData.height = scaledHeight;
        gNormal.type = TextureType::Color;

        auto albedoTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba8, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        albedoTex->setFilterMode(opal::TextureFilterMode::Nearest,
                                 opal::TextureFilterMode::Nearest);
        albedoTex->setWrapMode(opal::TextureAxis::S,
                               opal::TextureWrapMode::ClampToEdge);
        albedoTex->setWrapMode(opal::TextureAxis::T,
                               opal::TextureWrapMode::ClampToEdge);

        opal::Attachment albedoAttachment;
        albedoAttachment.texture = albedoTex;
        albedoAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(albedoAttachment);

        gAlbedoSpec.texture = albedoTex;
        gAlbedoSpec.creationData.width = scaledWidth;
        gAlbedoSpec.creationData.height = scaledHeight;
        gAlbedoSpec.type = TextureType::Color;

        auto materialTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgba8, width,
            height, opal::TextureDataFormat::Rgba, nullptr, 1);
        materialTex->setFilterMode(opal::TextureFilterMode::Nearest,
                                   opal::TextureFilterMode::Nearest);
        materialTex->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
        materialTex->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

        opal::Attachment materialAttachment;
        materialAttachment.texture = materialTex;
        materialAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(materialAttachment);

        gMaterial.texture = materialTex;
        gMaterial.creationData.width = scaledWidth;
        gMaterial.creationData.height = scaledHeight;
        gMaterial.type = TextureType::Color;

        auto gbufferDepth = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::DepthComponent24,
            width, height, opal::TextureDataFormat::DepthComponent, nullptr, 1);
        gbufferDepth->setFilterMode(opal::TextureFilterMode::Nearest,
                                    opal::TextureFilterMode::Nearest);
        gbufferDepth->setWrapMode(opal::TextureAxis::S,
                                  opal::TextureWrapMode::ClampToEdge);
        gbufferDepth->setWrapMode(opal::TextureAxis::T,
                                  opal::TextureWrapMode::ClampToEdge);

        opal::Attachment gbufferDepthAttachment;
        gbufferDepthAttachment.texture = gbufferDepth;
        gbufferDepthAttachment.type = opal::Attachment::Type::Depth;
        fb->addAttachment(gbufferDepthAttachment);

        depthTexture.texture = gbufferDepth;
        depthTexture.creationData.width = scaledWidth;
        depthTexture.creationData.height = scaledHeight;
        depthTexture.type = TextureType::Depth;

        if (!fb->getStatus()) {
            std::cerr << "Error: GBuffer Framebuffer is not complete!"
                      << std::endl;
        }

        fb->unbind();
    } else if (this->type == RenderTargetType::SSAO) {
        fb = opal::Framebuffer::create(width, height);

        auto ssaoTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Red8, width,
            height, opal::TextureDataFormat::Red, nullptr, 1);
        ssaoTex->setFilterMode(opal::TextureFilterMode::Nearest,
                               opal::TextureFilterMode::Nearest);
        ssaoTex->setWrapMode(opal::TextureAxis::S,
                             opal::TextureWrapMode::ClampToEdge);
        ssaoTex->setWrapMode(opal::TextureAxis::T,
                             opal::TextureWrapMode::ClampToEdge);

        opal::Attachment ssaoAttachment;
        ssaoAttachment.texture = ssaoTex;
        ssaoAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(ssaoAttachment);

        if (!fb->getStatus()) {
            std::cerr << "Error: SSAO Framebuffer is not complete!"
                      << std::endl;
        }

        texture.texture = ssaoTex;
        texture.creationData.width = scaledWidth;
        texture.creationData.height = scaledHeight;
        texture.type = TextureType::SSAO;

        fb->unbind();
    } else if (this->type == RenderTargetType::SSAOBlur) {
        fb = opal::Framebuffer::create(width, height);

        auto ssaoBlurTex = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Red8, width,
            height, opal::TextureDataFormat::Red, nullptr, 1);
        ssaoBlurTex->setFilterMode(opal::TextureFilterMode::Nearest,
                                   opal::TextureFilterMode::Nearest);
        ssaoBlurTex->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
        ssaoBlurTex->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

        opal::Attachment ssaoBlurAttachment;
        ssaoBlurAttachment.texture = ssaoBlurTex;
        ssaoBlurAttachment.type = opal::Attachment::Type::Color;
        fb->addAttachment(ssaoBlurAttachment);

        if (!fb->getStatus()) {
            std::cerr << "Error: SSAO Blur Framebuffer is not complete!"
                      << std::endl;
        }

        texture.texture = ssaoBlurTex;
        texture.creationData.width = scaledWidth;
        texture.creationData.height = scaledHeight;
        texture.type = TextureType::SSAO;

        fb->unbind();
    } else {
        atlas_warning("Unknown render target type");
        return;
    }

    AllocationPacket packet;
    packet.description =
        "RenderTarget Type " + std::to_string(static_cast<int>(type));
    ;
    packet.sizeMb = (static_cast<float>(width) * static_cast<float>(height) *
                     4.0f /* bytes per pixel */) /
                    (1024.0f * 1024.0f);
    packet.kind = DebugResourceKind::RenderTarget;
    packet.frameNumber = Window::mainWindow->device->frameCount;
    packet.send();
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
    if (type == RenderTargetType::Multisampled && fb && resolveFb) {
        for (int i = 0; i < 2; i++) {
            auto resolveAction =
                opal::ResolveAction::createForColorAttachment(fb, resolveFb, i);
            auto commandBuffer =
                Window::mainWindow->device->acquireCommandBuffer();
            commandBuffer->performResolve(resolveAction);
        }

        auto depthResolveAction =
            opal::ResolveAction::createForDepth(fb, resolveFb);
        auto commandBuffer = Window::mainWindow->device->acquireCommandBuffer();
        commandBuffer->performResolve(depthResolveAction);
    }

    if (type != RenderTargetType::Scene &&
        type != RenderTargetType::Multisampled) {
        return;
    }

    if (this->texture.texture != nullptr) {
        this->texture.texture->automaticallyGenerateMipmaps();
        this->texture.texture->setFilterMode(
            opal::TextureFilterMode::LinearMipmapLinear,
            opal::TextureFilterMode::Linear);
    }
}

void RenderTarget::bind() {
    if (fb) {
        fb->bind();
        fb->setViewport();
    } else {
        auto defaultFb =
            Window::mainWindow->getDevice()->getDefaultFramebuffer();
        defaultFb->bind();
        defaultFb->setViewport(0, 0, texture.creationData.width,
                               texture.creationData.height);
    }
}

void RenderTarget::bindCubemapFace(int face) {
    if (fb && texture.texture != nullptr) {
        fb->attachCubemapFace(texture.texture, face,
                              opal::Attachment::Type::Depth);
        fb->bind();
        fb->setViewport();
    }
}

void RenderTarget::unbind() {
    if (fb) {
        fb->unbind();
    }
}

std::shared_ptr<opal::Framebuffer> RenderTarget::getFramebuffer() const {
    return fb;
}

std::shared_ptr<opal::Framebuffer> RenderTarget::getResolveFramebuffer() const {
    return resolveFb;
}

int RenderTarget::getWidth() const {
    switch (type) {
    case RenderTargetType::Multisampled:
        return std::max(1, msTexture.creationData.width != 0
                               ? msTexture.creationData.width
                               : texture.creationData.width);
    case RenderTargetType::GBuffer:
        return std::max(1, gPosition.creationData.width);
    default:
        if (texture.creationData.width > 0) {
            return texture.creationData.width;
        }
        if (type == RenderTargetType::Scene &&
            brightTexture.creationData.width > 0) {
            return brightTexture.creationData.width;
        }
        if (type == RenderTargetType::Scene &&
            gMaterial.creationData.width > 0) {
            return gMaterial.creationData.width;
        }
        return 1;
    }
}

int RenderTarget::getHeight() const {
    switch (type) {
    case RenderTargetType::Multisampled:
        return std::max(1, msTexture.creationData.height != 0
                               ? msTexture.creationData.height
                               : texture.creationData.height);
    case RenderTargetType::GBuffer:
        return std::max(1, gPosition.creationData.height);
    default:
        if (texture.creationData.height > 0) {
            return texture.creationData.height;
        }
        if (type == RenderTargetType::Scene &&
            brightTexture.creationData.height > 0) {
            return brightTexture.creationData.height;
        }
        if (type == RenderTargetType::Scene &&
            gMaterial.creationData.height > 0) {
            return gMaterial.creationData.height;
        }
        return 1;
    }
}

void RenderTarget::hide() {
    if (object != nullptr) {
        object->hide();
    } else {
        atlas_error("Render target object is null");
    }
}

void RenderTarget::show() {
    if (object != nullptr) {
        object->show();
    } else {
        atlas_error("Render target object is null");
    }
}

void RenderTarget::render(float dt,
                          std::shared_ptr<opal::CommandBuffer> commandBuffer,
                          bool updatePipeline) {
    (void)updatePipeline;
    if (!object || !object->isVisible) {
        return;
    }
    if (commandBuffer == nullptr) {
        atlas_error("RenderTarget::render requires a valid command buffer");
        return;
    }

    CoreObject *obj = this->object.get();

    static std::shared_ptr<opal::Pipeline> renderTargetPipeline = nullptr;
    if (renderTargetPipeline == nullptr) {
        renderTargetPipeline = opal::Pipeline::create();
    }
    renderTargetPipeline =
        obj->shaderProgram.requestPipeline(renderTargetPipeline);
    renderTargetPipeline->bind();

    Camera *camera = Window::mainWindow->camera;

    if (texture.type == TextureType::DepthCube) {
        renderTargetPipeline->bindTextureCubemap("cubeMap", texture.id, 10,
                                                 obj->id);
        renderTargetPipeline->setUniform1i("isCubeMap", 1);
    } else {
        if (texture.id == 0) {
            renderTargetPipeline->bindTexture2D("Texture", gMaterial.id, 0,
                                                obj->id);
            renderTargetPipeline->setUniform1i("isCubeMap", 0);
        } else {
            renderTargetPipeline->bindTexture2D("Texture", texture.id, 0,
                                                obj->id);
            renderTargetPipeline->setUniform1i("isCubeMap", 0);
        }

        renderTargetPipeline->bindTexture2D("BrightTexture", blurredTexture.id,
                                            1, obj->id);
        renderTargetPipeline->setUniform1i("hasBrightTexture",
                                           brightTexture.id != 0 ? 1 : 0);

        renderTargetPipeline->bindTexture2D("DepthTexture", depthTexture.id, 2,
                                            obj->id);
        const bool hasDepth = depthTexture.id != 0;
        renderTargetPipeline->setUniform1i("hasDepthTexture", hasDepth ? 1 : 0);

        renderTargetPipeline->bindTexture2D(
            "VolumetricLightTexture", volumetricLightTexture.id, 3, obj->id);
        renderTargetPipeline->setUniform1i(
            "hasVolumetricLightTexture", volumetricLightTexture.id > 1 ? 1 : 0);

        renderTargetPipeline->bindTexture2D("PositionTexture", gPosition.id, 4,
                                            obj->id);
        renderTargetPipeline->setUniform1i("hasPositionTexture",
                                           gPosition.id != 0 ? 1 : 0);

        renderTargetPipeline->bindTexture2D("SSRTexture", ssrTexture.id, 5,
                                            obj->id);
        renderTargetPipeline->setUniform1i("hasSSRTexture",
                                           ssrTexture.id != 0 ? 1 : 0);

        renderTargetPipeline->bindTexture2D("LUTTexture", LUT.id, 6, obj->id);
        renderTargetPipeline->setUniform1i("hasLUTTexture",
                                           LUT.id != 0 ? 1 : 0);

        const glm::mat4 projectionMatrix =
            Window::mainWindow->calculateProjectionMatrix();
        const glm::mat4 viewMatrix =
            Window::mainWindow->getCamera()->calculateViewMatrix();

        renderTargetPipeline->setUniformMat4f("projectionMatrix",
                                              projectionMatrix);
        renderTargetPipeline->setUniformMat4f("invProjectionMatrix",
                                              glm::inverse(projectionMatrix));

        renderTargetPipeline->setUniformMat4f("viewMatrix", viewMatrix);
        renderTargetPipeline->setUniformMat4f("invViewMatrix",
                                              glm::inverse(viewMatrix));
        renderTargetPipeline->setUniformMat4f(
            "lastViewMatrix", Window::mainWindow->lastViewMatrix);
        renderTargetPipeline->setUniform3f("cameraPosition", camera->position.x,
                                           camera->position.y,
                                           camera->position.z);

        renderTargetPipeline->setUniform1f("nearPlane", camera->nearClip);
        renderTargetPipeline->setUniform1f("farPlane", camera->farClip);
        renderTargetPipeline->setUniform1f("focusDepth", camera->focusDepth);
        renderTargetPipeline->setUniform1f("focusRange", camera->focusRange);

        renderTargetPipeline->setUniform1f("deltaTime", dt);
        renderTargetPipeline->setUniform1f("time",
                                           Window::mainWindow->getTime());

        int maxMipLevels = (int)std::floor(
            std::log2(std::max(Window::mainWindow->getSize().width,
                               Window::mainWindow->getSize().height)));

        renderTargetPipeline->setUniform1i("maxMipLevel", maxMipLevels);

        Scene *scene = Window::mainWindow->getCurrentScene();
        renderTargetPipeline->setUniform1f("environment.fogIntensity",
                                           scene->environment.fog.intensity);
        renderTargetPipeline->setUniform3f(
            "environment.fogColor", scene->environment.fog.color.r,
            scene->environment.fog.color.g, scene->environment.fog.color.b);

        if (scene->atmosphere.clouds) {
            const Clouds &cloudSettings = *scene->atmosphere.clouds;

            const glm::vec3 cloudSize = cloudSettings.size.toGlm();
            const glm::vec3 cloudPos = cloudSettings.position.toGlm();

            glm::vec3 sunDir = scene->atmosphere.getSunAngle().toGlm();
            float sunLength = glm::length(sunDir);
            if (sunLength > 1e-3f) {
                sunDir /= sunLength;
            } else {
                sunDir = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            const Color &sunColor = scene->atmosphere.sunColor;
            const float sunIntensity = scene->atmosphere.getLightIntensity();
            const Color ambientColor = scene->getAmbientColor();
            const float ambientIntensity = scene->getAmbientIntensity();
            glm::vec3 ambient = glm::vec3(static_cast<float>(ambientColor.r),
                                          static_cast<float>(ambientColor.g),
                                          static_cast<float>(ambientColor.b)) *
                                ambientIntensity;

            renderTargetPipeline->bindTexture3D(
                "cloudsTexture", cloudSettings.getCloudTexture(128), 15,
                obj->id);
            renderTargetPipeline->setUniform3f("cloudSize", cloudSize.x,
                                               cloudSize.y, cloudSize.z);
            renderTargetPipeline->setUniform3f("cloudPosition", cloudPos.x,
                                               cloudPos.y, cloudPos.z);
            renderTargetPipeline->setUniform1f("cloudScale",
                                               cloudSettings.scale);
            renderTargetPipeline->setUniform3f(
                "cloudOffset", cloudSettings.offset.x, cloudSettings.offset.y,
                cloudSettings.offset.z);
            renderTargetPipeline->setUniform1f("cloudDensityThreshold",
                                               cloudSettings.density);
            renderTargetPipeline->setUniform1f("cloudDensityMultiplier",
                                               cloudSettings.densityMultiplier);
            renderTargetPipeline->setUniform1f("cloudAbsorption",
                                               cloudSettings.absorption);
            renderTargetPipeline->setUniform1f("cloudScattering",
                                               cloudSettings.scattering);
            renderTargetPipeline->setUniform1f("cloudPhaseG",
                                               cloudSettings.phase);
            renderTargetPipeline->setUniform1f("cloudClusterStrength",
                                               cloudSettings.clusterStrength);
            renderTargetPipeline->setUniform1i(
                "cloudPrimarySteps",
                std::max(1, cloudSettings.primaryStepCount));
            renderTargetPipeline->setUniform1i(
                "cloudLightSteps", std::max(1, cloudSettings.lightStepCount));
            renderTargetPipeline->setUniform1f(
                "cloudLightStepMultiplier", cloudSettings.lightStepMultiplier);
            renderTargetPipeline->setUniform1f("cloudMinStepLength",
                                               cloudSettings.minStepLength);
            renderTargetPipeline->setUniform3f("sunDirection", sunDir.x,
                                               sunDir.y, sunDir.z);
            renderTargetPipeline->setUniform3f(
                "sunColor", static_cast<float>(sunColor.r),
                static_cast<float>(sunColor.g), static_cast<float>(sunColor.b));
            renderTargetPipeline->setUniform1f("sunIntensity", sunIntensity);
            renderTargetPipeline->setUniform3f("cloudAmbientColor", ambient.x,
                                               ambient.y, ambient.z);
            renderTargetPipeline->setUniform1i("hasClouds", 1);
        } else {
            renderTargetPipeline->bindTexture3D("cloudsTexture", 0, 15,
                                                obj->id);
            renderTargetPipeline->setUniform1i("hasClouds", 0);
        }
    }

    renderTargetPipeline->setUniform1i("TextureType",
                                       static_cast<int>(texture.type));
    renderTargetPipeline->setUniform1i("EffectCount", effects.size());

    for (size_t i = 0; i < effects.size(); i++) {
        std::string uniformName = "Effects[" + std::to_string(i) + "]";
        renderTargetPipeline->setUniform1i(uniformName,
                                           static_cast<int>(effects[i]->type));
        effects[i]->applyToProgram(obj->shaderProgram, i);
    }

    renderTargetPipeline->enableDepthTest(false);
    renderTargetPipeline->enableBlending(true);
    renderTargetPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                       opal::BlendFunc::OneMinusSrcAlpha);
    renderTargetPipeline->bind();

    commandBuffer->bindDrawingState(obj->vao);
    if (!obj->indices.empty()) {
        commandBuffer->drawIndexed(
            static_cast<unsigned int>(obj->indices.size()), 1, 0, 0, 0,
            obj->id);
    } else {
        commandBuffer->draw(static_cast<unsigned int>(obj->vertices.size()), 1,
                            0, 0, obj->id);
    }
    commandBuffer->unbindDrawingState();

    renderTargetPipeline->enableDepthTest(true);
    renderTargetPipeline->bind();

    DebugObjectPacket debugPacket{};
    debugPacket.drawCallsForObject = 1;
    debugPacket.triangleCount = 2;
    debugPacket.vertexBufferSizeMb =
        static_cast<float>(sizeof(CoreVertex) * 4) / (1024.0f * 1024.0f);
    debugPacket.indexBufferSizeMb =
        static_cast<float>(sizeof(Index) * 6) / (1024.0f * 1024.0f);
    debugPacket.textureCount =
        1 + (brightTexture.id != 0 ? 1 : 0) + (depthTexture.id != 0 ? 1 : 0) +
        (gPosition.id != 0 ? 1 : 0) + (ssrTexture.id != 0 ? 1 : 0) +
        (LUT.id != 0 ? 1 : 0);
    debugPacket.materialCount = 0;
    debugPacket.objectType = DebugObjectType::Other;
    debugPacket.objectId = obj->id;

    debugPacket.send();
}
