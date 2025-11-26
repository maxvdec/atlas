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

void Window::deferredRendering(
    RenderTarget *target, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }
    // Render to G-Buffer
    std::vector<std::shared_ptr<opal::Pipeline>> originalPipelines;
    auto deferredPipeline = opal::Pipeline::create();
    for (auto &obj : this->renderables) {
        if (!obj->canUseDeferredRendering()) {
            continue;
        }
        if (obj->getPipeline() != std::nullopt) {
            originalPipelines.push_back(obj->getPipeline().value());
        } else {
            originalPipelines.push_back(opal::Pipeline::create());
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, this->gBuffer->fbo);
    glViewport(0, 0, this->gBuffer->getWidth(), this->gBuffer->getHeight());
    deferredPipeline->setViewport(0, 0, this->gBuffer->getWidth(),
                                  this->gBuffer->getHeight());
    unsigned int attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    deferredPipeline->setCullMode(opal::CullMode::Back);
    deferredPipeline->enableDepthTest(true);
    deferredPipeline->setDepthCompareOp(opal::CompareOp::Less);
    glDepthMask(GL_TRUE);
    deferredPipeline = this->deferredProgram.requestPipeline(deferredPipeline);
    for (auto &obj : this->renderables) {
        if (obj->canUseDeferredRendering()) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->setPipeline(deferredPipeline);
            obj->render(getDeltaTime(), commandBuffer, false);
        }
    }

    this->gBuffer->resolve();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    target->resolve();
    this->renderSSAO();

    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glViewport(0, 0, target->getWidth(), target->getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    unsigned int targetAttachments[2] = {GL_COLOR_ATTACHMENT0,
                                         GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, targetAttachments);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glDepthMask(GL_FALSE);

    static std::shared_ptr<opal::DrawingState> quadState = nullptr;
    static std::shared_ptr<opal::Buffer> quadBuffer = nullptr;
    if (quadState == nullptr) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };

        quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        quadState = opal::DrawingState::create(quadBuffer);
        quadState->setBuffers(quadBuffer, nullptr);

        opal::VertexAttribute positionAttr{"deferredPosition",
                                           opal::VertexAttributeType::Float,
                                           0,
                                           0,
                                           false,
                                           3,
                                           static_cast<uint>(5 * sizeof(float)),
                                           opal::VertexBindingInputRate::Vertex,
                                           0};
        opal::VertexAttribute uvAttr{"deferredUV",
                                     opal::VertexAttributeType::Float,
                                     static_cast<uint>(3 * sizeof(float)),
                                     1,
                                     false,
                                     2,
                                     static_cast<uint>(5 * sizeof(float)),
                                     opal::VertexBindingInputRate::Vertex,
                                     0};

        std::vector<opal::VertexAttributeBinding> bindings = {
            {positionAttr, quadBuffer}, {uvAttr, quadBuffer}};
        quadState->configureAttributes(bindings);
    }

    // Create and configure light pass pipeline
    static std::shared_ptr<opal::Pipeline> lightPipeline = nullptr;
    if (lightPipeline == nullptr) {
        lightPipeline = opal::Pipeline::create();
    }
    lightPipeline = this->lightProgram.requestPipeline(lightPipeline);
    lightPipeline->bind();

    lightPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id, 0);
    lightPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
    lightPipeline->bindTexture2D("gAlbedoSpec", this->gBuffer->gAlbedoSpec.id,
                                 2);
    lightPipeline->bindTexture2D("gMaterial", this->gBuffer->gMaterial.id, 3);

    int boundTextures = 4;

    if (this->ssaoBlurBuffer != nullptr) {
        lightPipeline->bindTexture2D("ssao", this->ssaoBlurBuffer->texture.id,
                                     4);
        boundTextures++;
    }

    int boundCubemaps = 0;

    Scene *scene = this->currentScene;

    lightPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                getCamera()->position.y,
                                getCamera()->position.z);

    // Set ambient light
    Color ambientColor = scene->getAmbientColor();
    float ambientIntensity = scene->getAmbientIntensity();
    if (scene->isAutomaticAmbientEnabled()) {
        ambientColor = scene->getAutomaticAmbientColor();
        ambientIntensity = scene->getAutomaticAmbientIntensity();
    }
    lightPipeline->setUniform4f("ambientLight.color",
                                static_cast<float>(ambientColor.r),
                                static_cast<float>(ambientColor.g),
                                static_cast<float>(ambientColor.b), 1.0f);
    lightPipeline->setUniform1f("ambientLight.intensity",
                                ambientIntensity / 2.0f);

    // Set camera position
    lightPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                getCamera()->position.y,
                                getCamera()->position.z);

    // Send directional lights
    int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
    lightPipeline->setUniform1i("directionalLightCount", dirLightCount);

    for (int i = 0; i < dirLightCount; i++) {
        DirectionalLight *light = scene->directionalLights[i];
        std::string baseName = "directionalLights[" + std::to_string(i) + "]";
        lightPipeline->setUniform3f(baseName + ".direction", light->direction.x,
                                    light->direction.y, light->direction.z);
        lightPipeline->setUniform3f(baseName + ".diffuse", light->color.r,
                                    light->color.g, light->color.b);
        lightPipeline->setUniform3f(baseName + ".specular", light->shineColor.r,
                                    light->shineColor.g, light->shineColor.b);
    }

    // Send point lights

    int pointLightCount = std::min((int)scene->pointLights.size(), 256);
    lightPipeline->setUniform1i("pointLightCount", pointLightCount);

    for (int i = 0; i < pointLightCount; i++) {
        Light *light = scene->pointLights[i];
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        lightPipeline->setUniform3f(baseName + ".position", light->position.x,
                                    light->position.y, light->position.z);
        lightPipeline->setUniform3f(baseName + ".diffuse", light->color.r,
                                    light->color.g, light->color.b);
        lightPipeline->setUniform3f(baseName + ".specular", light->shineColor.r,
                                    light->shineColor.g, light->shineColor.b);

        PointLightConstants plc = light->calculateConstants();
        lightPipeline->setUniform1f(baseName + ".constant", plc.constant);
        lightPipeline->setUniform1f(baseName + ".linear", plc.linear);
        lightPipeline->setUniform1f(baseName + ".quadratic", plc.quadratic);
        lightPipeline->setUniform1f(baseName + ".radius", plc.radius);
    }

    // Send spotlights

    int spotlightCount = std::min((int)scene->spotlights.size(), 256);
    lightPipeline->setUniform1i("spotlightCount", spotlightCount);

    for (int i = 0; i < spotlightCount; i++) {
        Spotlight *light = scene->spotlights[i];
        std::string baseName = "spotlights[" + std::to_string(i) + "]";
        lightPipeline->setUniform3f(baseName + ".position", light->position.x,
                                    light->position.y, light->position.z);
        lightPipeline->setUniform3f(baseName + ".direction", light->direction.x,
                                    light->direction.y, light->direction.z);
        lightPipeline->setUniform3f(baseName + ".diffuse", light->color.r,
                                    light->color.g, light->color.b);
        lightPipeline->setUniform3f(baseName + ".specular", light->shineColor.r,
                                    light->shineColor.g, light->shineColor.b);
        lightPipeline->setUniform1f(baseName + ".cutOff", light->cutOff);
        lightPipeline->setUniform1f(baseName + ".outerCutOff",
                                    light->outerCutoff);
    }

    // Send area lights
    int areaLightCount = std::min((int)scene->areaLights.size(), 256);
    lightPipeline->setUniform1i("areaLightCount", areaLightCount);

    for (int i = 0; i < areaLightCount; i++) {
        AreaLight *light = scene->areaLights[i];
        std::string baseName = "areaLights[" + std::to_string(i) + "]";
        lightPipeline->setUniform3f(baseName + ".position", light->position.x,
                                    light->position.y, light->position.z);
        lightPipeline->setUniform3f(baseName + ".right", light->right.x,
                                    light->right.y, light->right.z);
        lightPipeline->setUniform3f(baseName + ".up", light->up.x, light->up.y,
                                    light->up.z);
        lightPipeline->setUniform2f(baseName + ".size",
                                    static_cast<float>(light->size.width),
                                    static_cast<float>(light->size.height));
        lightPipeline->setUniform3f(baseName + ".diffuse", light->color.r,
                                    light->color.g, light->color.b);
        lightPipeline->setUniform3f(baseName + ".specular", light->shineColor.r,
                                    light->shineColor.g, light->shineColor.b);
        lightPipeline->setUniform1f(baseName + ".angle", light->angle);
        lightPipeline->setUniform1i(baseName + ".castsBothSides",
                                    light->castsBothSides ? 1 : 0);
    }

    for (int i = 0; i < 5; i++) {
        std::string uniformName = "cubeMap" + std::to_string(i + 1);
        lightPipeline->setUniform1i(uniformName, i + 10);
    }

    int shadow2DSamplerIndex = 0;
    int boundParameters = 0;

    // Cycle though directional lights
    for (auto light : scene->directionalLights) {
        if (!light->doesCastShadows) {
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
        lightPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
        lightPipeline->setUniform1i(baseName + ".textureIndex",
                                    shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        lightPipeline->setUniformMat4f(baseName + ".lightView",
                                       shadowParams.lightView);
        lightPipeline->setUniformMat4f(baseName + ".lightProjection",
                                       shadowParams.lightProjection);
        lightPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        lightPipeline->setUniform1f(baseName + ".isPointLight", 0);

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    // Cycle though spotlights
    for (auto light : scene->spotlights) {
        if (!light->doesCastShadows) {
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
        lightPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
        lightPipeline->setUniform1i(baseName + ".textureIndex",
                                    shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        lightPipeline->setUniformMat4f(baseName + ".lightView",
                                       shadowParams.lightView);
        lightPipeline->setUniformMat4f(baseName + ".lightProjection",
                                       shadowParams.lightProjection);
        lightPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        lightPipeline->setUniform1f(baseName + ".isPointLight", 0);

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    for (auto light : scene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        // Reserve up to 5 cubemap units (cubeMap1..cubeMap5 bound to
        // units 10..14)
        if (boundCubemaps >= 5) {
            break;
        }

        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        lightPipeline->bindTextureCubemap(
            "cubeMap" + std::to_string(boundCubemaps + 1),
            light->shadowRenderTarget->texture.id, 10 + boundCubemaps);
        lightPipeline->setUniform1i(baseName + ".textureIndex", boundCubemaps);
        lightPipeline->setUniform1f(baseName + ".farPlane", light->distance);
        lightPipeline->setUniform3f(baseName + ".lightPos", light->position.x,
                                    light->position.y, light->position.z);
        lightPipeline->setUniform1i(baseName + ".isPointLight", 1);

        boundParameters++;
        boundCubemaps++;
    }

    lightPipeline->setUniform1i("shadowParamCount", boundParameters);

    // Set texture units array using pipeline
    for (int i = 0; i < boundTextures && i < 16; i++) {
        std::string uniformName = "textures[" + std::to_string(i) + "]";
        lightPipeline->setUniform1i(uniformName, i);
    }

    // Bind skybox
    if (scene->skybox != nullptr) {
        lightPipeline->bindTextureCubemap("skybox", scene->skybox->cubemap.id,
                                          boundTextures);
        boundTextures++;
    }

    lightPipeline->setUniform1f(
        "environment.rimLightIntensity",
        Window::mainWindow->currentScene->environment.rimLight.intensity);
    lightPipeline->setUniform3f(
        "environment.rimLightColor",
        Window::mainWindow->currentScene->environment.rimLight.color.r,
        Window::mainWindow->currentScene->environment.rimLight.color.g,
        Window::mainWindow->currentScene->environment.rimLight.color.b);

    quadState->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    quadState->unbind();

    if (dirLightCount > 0) {
        if (!volumetricBuffer) {
            volumetricBuffer =
                std::make_shared<RenderTarget>(*this, RenderTargetType::Scene);
        }

        target->resolve();

        glBindFramebuffer(GL_FRAMEBUFFER, volumetricBuffer->fbo);
        glViewport(0, 0, volumetricBuffer->getWidth(),
                   volumetricBuffer->getHeight());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Create volumetric pipeline
        static std::shared_ptr<opal::Pipeline> volumetricPipeline = nullptr;
        if (volumetricPipeline == nullptr) {
            volumetricPipeline = opal::Pipeline::create();
        }
        volumetricPipeline =
            this->volumetricProgram.requestPipeline(volumetricPipeline);
        volumetricPipeline->bind();

        DirectionalLight *dirLight = scene->directionalLights[0];

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

        quadState->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadState->unbind();
    }

    if (this->ssrFramebuffer != nullptr && useSSR) {
        target->resolve();

        glBindFramebuffer(GL_FRAMEBUFFER, ssrFramebuffer->fbo);
        glViewport(0, 0, ssrFramebuffer->getWidth(),
                   ssrFramebuffer->getHeight());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Create SSR pipeline
        static std::shared_ptr<opal::Pipeline> ssrPipeline = nullptr;
        if (ssrPipeline == nullptr) {
            ssrPipeline = opal::Pipeline::create();
        }
        ssrPipeline = this->ssrProgram.requestPipeline(ssrPipeline);
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

        quadState->bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadState->unbind();
    }

    target->volumetricLightTexture = volumetricBuffer->texture;
    target->ssrTexture = ssrFramebuffer->texture;
    target->gPosition = gBuffer->gPosition;

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
