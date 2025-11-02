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
#include <glad/glad.h>
#include <iostream>

void Window::deferredRendering(RenderTarget *target) {
    // Render to G-Buffer
    std::vector<ShaderProgram> originalPrograms;
    for (auto &obj : this->renderables) {
        if (!obj->canUseDeferredRendering()) {
            continue;
        }
        if (obj->getShaderProgram() != std::nullopt) {
            originalPrograms.push_back(obj->getShaderProgram().value());
        } else {
            originalPrograms.push_back(ShaderProgram());
        }
        obj->setShader(this->deferredProgram);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, this->gBuffer->fbo);
    glViewport(0, 0, this->gBuffer->getWidth(), this->gBuffer->getHeight());
    unsigned int attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    for (auto &obj : this->renderables) {
        if (obj->canUseDeferredRendering()) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime());
        }
    }

    this->gBuffer->resolve();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    target->resolve();
    this->renderSSAO(target);

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

    static Id quadVAO = 0;
    static Id quadVBO;
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions         // texCoords
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right

            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f  // top-right
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
    }

    glUseProgram(this->lightProgram.programId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gPosition.id);
    this->lightProgram.setUniform1i("gPosition", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gNormal.id);
    this->lightProgram.setUniform1i("gNormal", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gAlbedoSpec.id);
    this->lightProgram.setUniform1i("gAlbedoSpec", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, this->gBuffer->gMaterial.id);
    this->lightProgram.setUniform1i("gMaterial", 3);

    int boundTextures = 4;

    if (this->ssaoBlurBuffer != nullptr) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, this->ssaoBlurBuffer->texture.id);
        this->lightProgram.setUniform1i("ssao", 4);
        boundTextures++;
    }

    int boundCubemaps = 0;

    ShaderProgram shaderProgram = this->lightProgram;
    Scene *scene = this->currentScene;

    shaderProgram.setUniform3f("cameraPosition", getCamera()->position.x,
                               getCamera()->position.y,
                               getCamera()->position.z);

    const bool shaderSupportsIbl =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::IBL) != shaderProgram.capabilities.end();

    // Set ambient light
    Color ambientColor = scene->getAmbientColor();
    float ambientIntensity = scene->getAmbientIntensity();
    if (scene->isAutomaticAmbientEnabled()) {
        ambientColor = scene->getAutomaticAmbientColor();
        ambientIntensity = scene->getAutomaticAmbientIntensity();
    }
    shaderProgram.setUniform4f("ambientLight.color",
                               static_cast<float>(ambientColor.r),
                               static_cast<float>(ambientColor.g),
                               static_cast<float>(ambientColor.b), 1.0f);
    shaderProgram.setUniform1f("ambientLight.intensity",
                               ambientIntensity / 2.0f);

    // Set camera position
    shaderProgram.setUniform3f("cameraPosition", getCamera()->position.x,
                               getCamera()->position.y,
                               getCamera()->position.z);

    // Send directional lights
    int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
    shaderProgram.setUniform1i("directionalLightCount", dirLightCount);

    for (int i = 0; i < dirLightCount; i++) {
        DirectionalLight *light = scene->directionalLights[i];
        std::string baseName = "directionalLights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".direction", light->direction.x,
                                   light->direction.y, light->direction.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);
    }

    // Send point lights

    int pointLightCount = std::min((int)scene->pointLights.size(), 256);
    shaderProgram.setUniform1i("pointLightCount", pointLightCount);

    for (int i = 0; i < pointLightCount; i++) {
        Light *light = scene->pointLights[i];
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".position", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);

        PointLightConstants plc = light->calculateConstants();
        shaderProgram.setUniform1f(baseName + ".constant", plc.constant);
        shaderProgram.setUniform1f(baseName + ".linear", plc.linear);
        shaderProgram.setUniform1f(baseName + ".quadratic", plc.quadratic);
        shaderProgram.setUniform1f(baseName + ".radius", plc.radius);
    }

    // Send spotlights

    int spotlightCount = std::min((int)scene->spotlights.size(), 256);
    shaderProgram.setUniform1i("spotlightCount", spotlightCount);

    for (int i = 0; i < spotlightCount; i++) {
        Spotlight *light = scene->spotlights[i];
        std::string baseName = "spotlights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".position", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform3f(baseName + ".direction", light->direction.x,
                                   light->direction.y, light->direction.z);
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);
        shaderProgram.setUniform1f(baseName + ".cutOff", light->cutOff);
        shaderProgram.setUniform1f(baseName + ".outerCutOff",
                                   light->outerCutoff);
    }

    // Send area lights
    int areaLightCount = std::min((int)scene->areaLights.size(), 256);
    shaderProgram.setUniform1i("areaLightCount", areaLightCount);

    for (int i = 0; i < areaLightCount; i++) {
        AreaLight *light = scene->areaLights[i];
        std::string baseName = "areaLights[" + std::to_string(i) + "]";
        shaderProgram.setUniform3f(baseName + ".position", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform3f(baseName + ".right", light->right.x,
                                   light->right.y, light->right.z);
        shaderProgram.setUniform3f(baseName + ".up", light->up.x, light->up.y,
                                   light->up.z);
        shaderProgram.setUniform2f(baseName + ".size",
                                   static_cast<float>(light->size.width),
                                   static_cast<float>(light->size.height));
        shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                   light->color.g, light->color.b);
        shaderProgram.setUniform3f(baseName + ".specular", light->shineColor.r,
                                   light->shineColor.g, light->shineColor.b);
        shaderProgram.setUniform1f(baseName + ".angle", light->angle);
        shaderProgram.setUniform1i(baseName + ".castsBothSides",
                                   light->castsBothSides ? 1 : 0);
    }

    for (int i = 0; i < 5; i++) {
        std::string uniformName = "cubeMap" + std::to_string(i + 1);
        shaderProgram.setUniform1i(uniformName, i + 10);
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
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i("texture" +
                                       std::to_string(shadow2DSamplerIndex + 1),
                                   boundTextures);
        shaderProgram.setUniform1i(baseName + ".textureIndex",
                                   shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        shaderProgram.setUniformMat4f(baseName + ".lightView",
                                      shadowParams.lightView);
        shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                      shadowParams.lightProjection);
        shaderProgram.setUniform1f(baseName + ".bias", shadowParams.bias);
        shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

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
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i("texture" +
                                       std::to_string(shadow2DSamplerIndex + 1),
                                   boundTextures);
        shaderProgram.setUniform1i(baseName + ".textureIndex",
                                   shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
        shaderProgram.setUniformMat4f(baseName + ".lightView",
                                      shadowParams.lightView);
        shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                      shadowParams.lightProjection);
        shaderProgram.setUniform1f(baseName + ".bias", shadowParams.bias);
        shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

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

        glActiveTexture(GL_TEXTURE0 + 10 + boundCubemaps);
        glBindTexture(GL_TEXTURE_CUBE_MAP,
                      light->shadowRenderTarget->texture.id);

        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        shaderProgram.setUniform1i(baseName + ".textureIndex", boundCubemaps);
        shaderProgram.setUniform1f(baseName + ".farPlane", light->distance);
        shaderProgram.setUniform3f(baseName + ".lightPos", light->position.x,
                                   light->position.y, light->position.z);
        shaderProgram.setUniform1i(baseName + ".isPointLight", 1);

        boundParameters++;
        boundCubemaps++;
    }

    shaderProgram.setUniform1i("shadowParamCount", boundParameters);

    GLint units[16];
    for (int i = 0; i < boundTextures; i++)
        units[i] = i;

    glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                 boundTextures, units);

    // Bind skybox
    if (scene->skybox != nullptr) {
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene->skybox->cubemap.id);
        shaderProgram.setUniform1i("skybox", boundTextures);
        boundTextures++;
    }

    shaderProgram.setUniform1f(
        "environment.rimLightIntensity",
        Window::mainWindow->currentScene->environment.rimLight.intensity);
    shaderProgram.setUniform3f(
        "environment.rimLightColor",
        Window::mainWindow->currentScene->environment.rimLight.color.r,
        Window::mainWindow->currentScene->environment.rimLight.color.g,
        Window::mainWindow->currentScene->environment.rimLight.color.b);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

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

        glUseProgram(volumetricProgram.programId);

        DirectionalLight *dirLight = scene->directionalLights[0];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, target->texture.id);
        volumetricProgram.setUniform1i("sceneTexture", 0);

        volumetricProgram.setUniform1f(
            "density", scene->environment.volumetricLighting.density);
        volumetricProgram.setUniform1f(
            "weight", scene->environment.volumetricLighting.weight);
        volumetricProgram.setUniform1f(
            "decay", scene->environment.volumetricLighting.decay);
        volumetricProgram.setUniform1f(
            "exposure", scene->environment.volumetricLighting.exposure);
        volumetricProgram.setUniform3f("directionalLight.color",
                                       dirLight->color.r, dirLight->color.g,
                                       dirLight->color.b);
        glm::vec3 lightPos = -dirLight->direction.toGlm() * 1000.f;
        glm::vec4 clipSpace = calculateProjectionMatrix() *
                              camera->calculateViewMatrix() *
                              glm::vec4(lightPos, 1.0f);

        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
        glm::vec2 sunUV = (glm::vec2(ndc.x, ndc.y) + 1.0f) * 0.5f;

        volumetricProgram.setUniform2f("sunPos", sunUV.x, sunUV.y);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    if (this->ssrFramebuffer != nullptr && useSSR) {
        target->resolve();

        glBindFramebuffer(GL_FRAMEBUFFER, ssrFramebuffer->fbo);
        glViewport(0, 0, ssrFramebuffer->getWidth(),
                   ssrFramebuffer->getHeight());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(ssrProgram.programId);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gPosition.id);
        ssrProgram.setUniform1i("gPosition", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gNormal.id);
        ssrProgram.setUniform1i("gNormal", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gAlbedoSpec.id);
        ssrProgram.setUniform1i("gAlbedoSpec", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gMaterial.id);
        ssrProgram.setUniform1i("gMaterial", 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, target->texture.id);
        ssrProgram.setUniform1i("sceneColor", 4);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, gBuffer->depthTexture.id);
        ssrProgram.setUniform1i("gDepth", 5);

        const glm::mat4 projectionMatrix =
            Window::mainWindow->calculateProjectionMatrix();
        const glm::mat4 viewMatrix =
            Window::mainWindow->getCamera()->calculateViewMatrix();
        ssrProgram.setUniformMat4f("projection", projectionMatrix);
        ssrProgram.setUniformMat4f("view", viewMatrix);
        ssrProgram.setUniformMat4f("inverseView", glm::inverse(viewMatrix));
        ssrProgram.setUniformMat4f("inverseProjection",
                                   glm::inverse(projectionMatrix));
        ssrProgram.setUniform3f("cameraPosition", camera->position.x,
                                camera->position.y, camera->position.z);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    target->volumetricLightTexture = volumetricBuffer->texture;
    target->ssrTexture = ssrFramebuffer->texture;
    target->gPosition = gBuffer->gPosition;

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
