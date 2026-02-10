//
// deferred.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Deferred rendering pipeline implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/light.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <glad/glad.h>
#include <memory>
#include <vector>

namespace {
    std::vector<GPUDirectionalLight>
    buildGPUDirectionalLights(const std::vector<DirectionalLight*>& lights,
                              int maxCount) {
        std::vector<GPUDirectionalLight> result;
        int count = std::min(static_cast<int>(lights.size()), maxCount);
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            DirectionalLight* light = lights[i];
            GPUDirectionalLight gpu{};
            gpu.direction = glm::vec3(light->direction.x, light->direction.y,
                                      light->direction.z);
            gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu._pad1 = 0.0f;
            gpu._pad2 = 0.0f;
            gpu._pad3 = 0.0f;
            result.push_back(gpu);
        }
        return result;
    }

    std::vector<GPUPointLight>
    buildGPUPointLights(const std::vector<Light*>& lights, int maxCount) {
        std::vector<GPUPointLight> result;
        int count = std::min(static_cast<int>(lights.size()), maxCount);
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            Light* light = lights[i];
            PointLightConstants plc = light->calculateConstants();
            GPUPointLight gpu{};
            gpu.position =
                glm::vec3(light->position.x, light->position.y, light->position.z);
            gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.constant = plc.constant;
            gpu.linear = plc.linear;
            gpu.quadratic = plc.quadratic;
            gpu.radius = plc.radius;
            gpu._pad1 = 0.0f;
            gpu._pad2 = 0.0f;
            gpu._pad3 = 0.0f;
            result.push_back(gpu);
        }
        return result;
    }

    std::vector<GPUSpotLight>
    buildGPUSpotLights(const std::vector<Spotlight*>& lights, int maxCount) {
        std::vector<GPUSpotLight> result;
        int count = std::min(static_cast<int>(lights.size()), maxCount);
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            Spotlight* light = lights[i];
            GPUSpotLight gpu{};
            gpu.position =
                glm::vec3(light->position.x, light->position.y, light->position.z);
            gpu.direction = glm::vec3(light->direction.x, light->direction.y,
                                      light->direction.z);
            gpu.cutOff = light->cutOff;
            gpu.outerCutOff = light->outerCutoff;
            gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu._pad1 = 0.0f;
            gpu._pad2 = 0.0f;
            gpu._pad3 = 0.0f;
            gpu._pad4 = 0.0f;
            gpu._pad5 = 0.0f;
            gpu._pad6 = 0.0f;
            result.push_back(gpu);
        }
        return result;
    }

    std::vector<GPUAreaLight>
    buildGPUAreaLights(const std::vector<AreaLight*>& lights, int maxCount) {
        std::vector<GPUAreaLight> result;
        int count = std::min(static_cast<int>(lights.size()), maxCount);
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            AreaLight* light = lights[i];
            GPUAreaLight gpu{};
            gpu.position =
                glm::vec3(light->position.x, light->position.y, light->position.z);
            gpu.right = glm::vec3(light->right.x, light->right.y, light->right.z);
            gpu.up = glm::vec3(light->up.x, light->up.y, light->up.z);
            gpu.size = glm::vec2(light->size.width, light->size.height);
            gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.angle = light->angle;
            gpu.castsBothSides = light->castsBothSides ? 1 : 0;
            gpu._pad1 = 0.0f;
            gpu._pad2 = 0.0f;
            gpu._pad3 = 0.0f;
            gpu._pad4 = 0.0f;
            gpu._pad5 = 0.0f;
            gpu._pad6 = 0.0f;
            gpu._pad7 = 0.0f;
            gpu._pad8 = 0.0f;
            gpu._pad9 = 0.0f;
            result.push_back(gpu);
        }
        return result;
    }
} // anonymous namespace

void Window::deferredRendering(
    RenderTarget* target, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }
    auto deferredPipeline = opal::Pipeline::create();

    commandBuffer->endPass();
    auto gBufferRenderPass = opal::RenderPass::create();
    gBufferRenderPass->setFramebuffer(this->gBuffer->getFramebuffer());
    commandBuffer->beginPass(gBufferRenderPass);

    this->gBuffer->bind();
    this->gBuffer->getFramebuffer()->setViewport(
        0, 0, this->gBuffer->getWidth(), this->gBuffer->getHeight());
    deferredPipeline->setViewport(0, 0, this->gBuffer->getWidth(),
                                  this->gBuffer->getHeight());
    this->gBuffer->getFramebuffer()->setDrawBuffers(4);
    commandBuffer->clear(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    deferredPipeline->setCullMode(opal::CullMode::Back);
#ifdef VULKAN
    // In Vulkan, the Y-flip in projection inverts winding order
    deferredPipeline->setFrontFace(opal::FrontFace::Clockwise);
#endif
    deferredPipeline->enableDepthTest(true);
    deferredPipeline->setDepthCompareOp(opal::CompareOp::Less);
    deferredPipeline->enableDepthWrite(true);
    deferredPipeline = this->deferredProgram.requestPipeline(deferredPipeline);
    for (auto& obj : this->renderables) {
        if (obj->canUseDeferredRendering()) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->setPipeline(deferredPipeline);
            obj->render(getDeltaTime(), commandBuffer, false);
        }
    }

    this->gBuffer->resolve(commandBuffer);

    commandBuffer->endPass();
    this->gBuffer->unbind();

    auto resolveDepth = opal::ResolveAction::createForDepth(
        this->gBuffer->getFramebuffer(), target->getFramebuffer());
    commandBuffer->performResolve(resolveDepth);

    this->renderSSAO(commandBuffer);

    auto targetRenderPass = opal::RenderPass::create();
    targetRenderPass->setFramebuffer(target->getFramebuffer());
    commandBuffer->beginPass(targetRenderPass);

    target->bind();
    target->getFramebuffer()->setViewport(0, 0, target->getWidth(),
                                          target->getHeight());
    commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 0.0f);

    target->getFramebuffer()->setDrawBuffers(2);

    static std::shared_ptr<opal::DrawingState> quadState = nullptr;
    static std::shared_ptr<opal::Buffer> quadBuffer = nullptr;
    const uint quadStride = static_cast<uint>(5 * sizeof(float));
    const opal::VertexAttribute positionAttr{
        .name = "deferredPosition",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 3,
        .stride = quadStride,
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0
    };
    const opal::VertexAttribute uvAttr{
        .name = "deferredUV",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<uint>(3 * sizeof(float)),
        .location = 1,
        .normalized = false,
        .size = 2,
        .stride = quadStride,
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0
    };
    const opal::VertexBinding quadBinding{
        quadStride,
        opal::VertexBindingInputRate::Vertex
    };
    if (quadState == nullptr) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f // top-right
        };

        quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        quadState = opal::DrawingState::create(quadBuffer);
        quadState->setBuffers(quadBuffer, nullptr);

        std::vector<opal::VertexAttributeBinding> bindings = {
            {positionAttr, quadBuffer}, {uvAttr, quadBuffer}
        };
        quadState->configureAttributes(bindings);
    }

    if (this->lightPassPipeline == nullptr) {
        this->lightPassPipeline = opal::Pipeline::create();
        this->lightPassPipeline->setShaderProgram(this->lightProgram.shader);
        this->lightPassPipeline->setCullMode(opal::CullMode::None);
        this->lightPassPipeline->enableDepthTest(false);
        this->lightPassPipeline->enableDepthWrite(false);

        // The deferred light pass only needs Position(0) and UV(1).
        std::vector<opal::VertexAttribute> quadAttributes = {
            positionAttr,
            uvAttr
        };
        this->lightPassPipeline->setVertexAttributes(quadAttributes,
                                                     quadBinding);
        this->lightPassPipeline->build();
    }
    this->lightPassPipeline->setViewport(0, 0, target->getWidth(), target->getHeight());
    this->lightPassPipeline->bind();

    this->lightPassPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id, 0);
    this->lightPassPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
    this->lightPassPipeline->bindTexture2D("gAlbedoSpec", this->gBuffer->gAlbedoSpec.id,
                                           2);
    this->lightPassPipeline->bindTexture2D("gMaterial", this->gBuffer->gMaterial.id, 3);

    int boundTextures = 4;

    if (this->ssaoBlurBuffer != nullptr) {
        this->lightPassPipeline->bindTexture2D("ssao", this->ssaoBlurBuffer->texture.id,
                                               4);
        boundTextures++;
    }

    int boundCubemaps = 0;
#ifdef VULKAN
    std::vector<GPUShadowParams> gpuShadowParams;
    gpuShadowParams.reserve(10);
#endif

    Scene* scene = this->currentScene;

    this->lightPassPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                          getCamera()->position.y,
                                          getCamera()->position.z);

    // Set ambient light
    Color ambientColor = scene->getAmbientColor();
    float ambientIntensity = scene->getAmbientIntensity();
    if (scene->isAutomaticAmbientEnabled()) {
        ambientColor = scene->getAutomaticAmbientColor();
        ambientIntensity = scene->getAutomaticAmbientIntensity();
    }
    this->lightPassPipeline->setUniform4f("ambientLight.color", ambientColor.r,
                                          ambientColor.g,
                                          static_cast<float>(ambientColor.b), 1.0f);
    this->lightPassPipeline->setUniform1f("ambientLight.intensity", ambientIntensity);

    // Set camera position
    this->lightPassPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                          getCamera()->position.y,
                                          getCamera()->position.z);

    // Send directional lights using buffer binding
    int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
    this->lightPassPipeline->setUniform1i("directionalLightCount", dirLightCount);

    if (dirLightCount > 0) {
        auto gpuDirLights =
            buildGPUDirectionalLights(scene->directionalLights, dirLightCount);
        this->lightPassPipeline->bindBuffer("DirectionalLights", gpuDirLights);
    }

    // Send point lights using buffer binding
    int pointLightCount = std::min((int)scene->pointLights.size(), 256);
    this->lightPassPipeline->setUniform1i("pointLightCount", pointLightCount);

    if (pointLightCount > 0) {
        auto gpuPointLights =
            buildGPUPointLights(scene->pointLights, pointLightCount);
        this->lightPassPipeline->bindBuffer("PointLights", gpuPointLights);
    }

    // Send spotlights using buffer binding
    int spotlightCount = std::min((int)scene->spotlights.size(), 256);
    this->lightPassPipeline->setUniform1i("spotlightCount", spotlightCount);

    if (spotlightCount > 0) {
        auto gpuSpotLights =
            buildGPUSpotLights(scene->spotlights, spotlightCount);
        this->lightPassPipeline->bindBuffer("SpotLights", gpuSpotLights);
    }

    // Send area lights using buffer binding
    int areaLightCount = std::min((int)scene->areaLights.size(), 256);
    this->lightPassPipeline->setUniform1i("areaLightCount", areaLightCount);

    if (areaLightCount > 0) {
        auto gpuAreaLights =
            buildGPUAreaLights(scene->areaLights, areaLightCount);
        this->lightPassPipeline->bindBuffer("AreaLights", gpuAreaLights);
    }

#ifndef VULKAN
    for (int i = 0; i < 5; i++) {
        std::string uniformName = "cubeMap" + std::to_string(i + 1);
        this->lightPassPipeline->setUniform1i(uniformName, i + 10);
    }
#endif

    int shadow2DSamplerIndex = 0;
    int boundParameters = 0;

    // Cycle though directional lights
    for (auto* light : scene->directionalLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (light->shadowRenderTarget == nullptr) {
            continue;
        }
        if (boundTextures >= 16) {
            break;
        }

        if (shadow2DSamplerIndex >= 5) {
            break;
        }
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        this->lightPassPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
#ifdef VULKAN
        GPUShadowParams gpu{};
        gpu.lightView = light->lastShadowParams.lightView;
        gpu.lightProjection = light->lastShadowParams.lightProjection;
        gpu.bias = light->lastShadowParams.bias;
        gpu.textureIndex = shadow2DSamplerIndex;
        gpu.farPlane = 0.0f;
        gpu._pad1 = 0.0f;
        gpu.lightPos = glm::vec3(0.0f);
        gpu.isPointLight = 0;
        gpuShadowParams.push_back(gpu);
#else
        this->lightPassPipeline->setUniform1i(baseName + ".textureIndex",
                                              shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        this->lightPassPipeline->setUniformMat4f(baseName + ".lightView",
                                                 shadowParams.lightView);
        this->lightPassPipeline->setUniformMat4f(baseName + ".lightProjection",
                                                 shadowParams.lightProjection);
        this->lightPassPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        this->lightPassPipeline->setUniform1f(baseName + ".isPointLight", 0);
#endif

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    // Cycle though spotlights
    for (auto* light : scene->spotlights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (light->shadowRenderTarget == nullptr) {
            continue;
        }
        if (boundTextures >= 16) {
            break;
        }

        if (shadow2DSamplerIndex >= 5) {
            break;
        }
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        this->lightPassPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
#ifdef VULKAN
        GPUShadowParams gpu{};
        gpu.lightView = light->lastShadowParams.lightView;
        gpu.lightProjection = light->lastShadowParams.lightProjection;
        gpu.bias = light->lastShadowParams.bias;
        gpu.textureIndex = shadow2DSamplerIndex;
        gpu.farPlane = 0.0f;
        gpu._pad1 = 0.0f;
        gpu.lightPos = glm::vec3(0.0f);
        gpu.isPointLight = 0;
        gpuShadowParams.push_back(gpu);
#else
        this->lightPassPipeline->setUniform1i(baseName + ".textureIndex",
                                              shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        this->lightPassPipeline->setUniformMat4f(baseName + ".lightView",
                                                 shadowParams.lightView);
        this->lightPassPipeline->setUniformMat4f(baseName + ".lightProjection",
                                                 shadowParams.lightProjection);
        this->lightPassPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        this->lightPassPipeline->setUniform1f(baseName + ".isPointLight", 0);
#endif

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    for (auto* light : scene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (light->shadowRenderTarget == nullptr) {
            continue;
        }
        if (boundCubemaps >= 5) {
            break;
        }

        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        this->lightPassPipeline->bindTextureCubemap(
            "cubeMap" + std::to_string(boundCubemaps + 1),
            light->shadowRenderTarget->texture.id, 10 + boundCubemaps);
#ifdef VULKAN
        GPUShadowParams gpu{};
        gpu.lightView = glm::mat4(1.0f);
        gpu.lightProjection = glm::mat4(1.0f);
        gpu.bias = 0.0f;
        gpu.textureIndex = boundCubemaps;
        gpu.farPlane = light->distance;
        gpu._pad1 = 0.0f;
        gpu.lightPos =
            glm::vec3(light->position.x, light->position.y, light->position.z);
        gpu.isPointLight = 1;
        gpuShadowParams.push_back(gpu);
#else
        this->lightPassPipeline->setUniform1i(baseName + ".textureIndex", boundCubemaps);
        this->lightPassPipeline->setUniform1f(baseName + ".farPlane", light->distance);
        this->lightPassPipeline->setUniform3f(baseName + ".lightPos", light->position.x,
                                              light->position.y, light->position.z);
        this->lightPassPipeline->setUniform1i(baseName + ".isPointLight", 1);
#endif

        boundParameters++;
        boundCubemaps++;
    }

    this->lightPassPipeline->setUniform1i("shadowParamCount", boundParameters);
#ifdef VULKAN
    if (!gpuShadowParams.empty()) {
        this->lightPassPipeline->bindBuffer("ShadowParams", gpuShadowParams);
    }
#endif

    // Set texture units array using pipeline
#ifndef VULKAN
    for (int i = 0; i < boundTextures && i < 16; i++) {
        std::string uniformName = "textures[" + std::to_string(i) + "]";
        this->lightPassPipeline->setUniform1i(uniformName, i);
    }
#endif

    bool hasSkybox = scene->skybox != nullptr;
    this->lightPassPipeline->setUniformBool("hasSkybox", hasSkybox);
    this->lightPassPipeline->setUniformBool("useIBL", false);
    if (hasSkybox) {
        this->lightPassPipeline->bindTextureCubemap("skybox", scene->skybox->cubemap.id,
                                                    boundTextures);
        boundTextures++;
    }
    else {
        this->lightPassPipeline->setUniformBool("useIBL", true);
    }

    this->lightPassPipeline->setUniform1f(
        "environment.rimLightIntensity",
        Window::mainWindow->currentScene->environment.rimLight.intensity);
    this->lightPassPipeline->setUniform3f(
        "environment.rimLightColor",
        Window::mainWindow->currentScene->environment.rimLight.color.r,
        Window::mainWindow->currentScene->environment.rimLight.color.g,
        Window::mainWindow->currentScene->environment.rimLight.color.b);

    commandBuffer->bindPipeline(this->lightPassPipeline);
    commandBuffer->bindDrawingState(quadState);
    commandBuffer->draw(6, 1, 0, 0);
    commandBuffer->unbindDrawingState();

    updatePipelineStateField(this->writeDepth, true);
    updatePipelineStateField(this->depthCompareOp, opal::CompareOp::LessEqual);
    updatePipelineStateField(this->cullMode, opal::CullMode::Back);
    updatePipelineStateField(this->cullMode, opal::CullMode::Back);

    for (auto& obj : this->firstRenderables) {
        obj->setViewMatrix(this->camera->calculateViewMatrix());
        obj->setProjectionMatrix(calculateProjectionMatrix());
        obj->render(getDeltaTime(), commandBuffer,
                    shouldRefreshPipeline(obj));
    }

    for (auto& obj : this->renderables) {
        if (obj->renderLateForward) {
            continue;
        }
        if (!obj->canUseDeferredRendering()) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
    }

    for (auto& obj : this->lateForwardRenderables) {
        obj->setViewMatrix(this->camera->calculateViewMatrix());
        obj->setProjectionMatrix(calculateProjectionMatrix());
        obj->render(getDeltaTime(), commandBuffer,
                    shouldRefreshPipeline(obj));
    }

    commandBuffer->endPass();
    target->resolve(commandBuffer);

    if (dirLightCount > 0) {
        if (!volumetricBuffer) {
            volumetricBuffer =
                std::make_shared<RenderTarget>(*this, RenderTargetType::Scene);
        }

        // Start volumetric pass
        auto volumetricRenderPass = opal::RenderPass::create();
        volumetricRenderPass->setFramebuffer(
            volumetricBuffer->getFramebuffer());
        commandBuffer->beginPass(volumetricRenderPass);

        volumetricBuffer->bind();
        volumetricBuffer->getFramebuffer()->setViewport(
            0, 0, volumetricBuffer->getWidth(), volumetricBuffer->getHeight());
        commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Create volumetric pipeline
        static std::shared_ptr<opal::Pipeline> volumetricPipeline = nullptr;
        if (volumetricPipeline == nullptr) {
            volumetricPipeline = opal::Pipeline::create();
            volumetricPipeline->setShaderProgram(
                this->volumetricProgram.shader);
            std::vector<opal::VertexAttribute> quadAttributes = {
                positionAttr,
                uvAttr
            };
            volumetricPipeline->setVertexAttributes(quadAttributes,
                                                    quadBinding);
            volumetricPipeline->build();
        }
        volumetricPipeline->setViewport(0, 0, volumetricBuffer->getWidth(),
                                        volumetricBuffer->getHeight());
        volumetricPipeline->bind();

        DirectionalLight* dirLight = scene->directionalLights[0];

        volumetricPipeline->bindTexture2D("sceneTexture", target->texture.id,
                                          0);

        volumetricPipeline->setUniform1f(
            "density", scene->environment.volumetricLighting.density);
        volumetricPipeline->setUniform1f(
            "weight", scene->environment.volumetricLighting.weight);
        volumetricPipeline->setUniform1f(
            "decay", scene->environment.volumetricLighting.decay);
        volumetricPipeline->setUniform1f(
            "exposure", scene->environment.volumetricLighting.exposure);
        volumetricPipeline->setUniform3f("directionalLight.color",
                                         dirLight->color.r, dirLight->color.g,
                                         dirLight->color.b);
        glm::vec3 lightPos = -dirLight->direction.toGlm() * 1000.f;
        glm::vec4 clipSpace = calculateProjectionMatrix() *
            camera->calculateViewMatrix() *
            glm::vec4(lightPos, 1.0f);

        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
        glm::vec2 sunUV = (glm::vec2(ndc.x, ndc.y) + 1.0f) * 0.5f;

        volumetricPipeline->setUniform2f("sunPos", sunUV.x, sunUV.y);

        commandBuffer->bindPipeline(volumetricPipeline);
        commandBuffer->bindDrawingState(quadState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();

        commandBuffer->endPass();
    }

    if (this->ssrFramebuffer != nullptr && useSSR) {
        auto ssrRenderPass = opal::RenderPass::create();
        ssrRenderPass->setFramebuffer(ssrFramebuffer->getFramebuffer());
        commandBuffer->beginPass(ssrRenderPass);

        ssrFramebuffer->bind();
        ssrFramebuffer->getFramebuffer()->setViewport(
            0, 0, ssrFramebuffer->getWidth(), ssrFramebuffer->getHeight());
        commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Create SSR pipeline
        static std::shared_ptr<opal::Pipeline> ssrPipeline = nullptr;
        if (ssrPipeline == nullptr) {
            ssrPipeline = opal::Pipeline::create();
            ssrPipeline->setShaderProgram(this->ssrProgram.shader);
            std::vector<opal::VertexAttribute> quadAttributes = {
                positionAttr,
                uvAttr
            };
            ssrPipeline->setVertexAttributes(quadAttributes, quadBinding);
            ssrPipeline->build();
        }
        ssrPipeline->setViewport(0, 0, ssrFramebuffer->getWidth(),
                                 ssrFramebuffer->getHeight());
        ssrPipeline->bind();

        ssrPipeline->bindTexture2D("gPosition", gBuffer->gPosition.id, 0);
        ssrPipeline->bindTexture2D("gNormal", gBuffer->gNormal.id, 1);
        ssrPipeline->bindTexture2D("gAlbedoSpec", gBuffer->gAlbedoSpec.id, 2);
        ssrPipeline->bindTexture2D("gMaterial", gBuffer->gMaterial.id, 3);
        ssrPipeline->bindTexture2D("sceneColor", target->texture.id, 4);
        ssrPipeline->bindTexture2D("gDepth", gBuffer->depthTexture.id, 5);

        const glm::mat4 projectionMatrix =
            Window::mainWindow->calculateProjectionMatrix();
        const glm::mat4 viewMatrix =
            Window::mainWindow->getCamera()->calculateViewMatrix();
        ssrPipeline->setUniformMat4f("projection", projectionMatrix);
        ssrPipeline->setUniformMat4f("view", viewMatrix);
        ssrPipeline->setUniformMat4f("inverseView", glm::inverse(viewMatrix));
        ssrPipeline->setUniformMat4f("inverseProjection",
                                     glm::inverse(projectionMatrix));
        ssrPipeline->setUniform3f("cameraPosition", camera->position.x,
                                  camera->position.y, camera->position.z);

        commandBuffer->bindPipeline(ssrPipeline);
        commandBuffer->bindDrawingState(quadState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();

        commandBuffer->endPass();
    }

    target->volumetricLightTexture = volumetricBuffer->texture;
    target->ssrTexture = ssrFramebuffer->texture;
    target->gPosition = gBuffer->gPosition;
}
