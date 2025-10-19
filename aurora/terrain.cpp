//
// terrain.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Terrain functions implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <cstdint>
#include <glad/glad.h>
#include "atlas/camera.h"
#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <aurora/terrain.h>
#include <iostream>
#include <vector>
#include "stb/stb_image.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Terrain::initialize() {
    while (glGetError() != GL_NO_ERROR) {
    }

    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Terrain);
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Terrain);
    TessellationShader tescShader = TessellationShader::fromDefaultShader(
        AtlasTessellationShader::TerrainControl);
    TessellationShader teseShader = TessellationShader::fromDefaultShader(
        AtlasTessellationShader::TerrainEvaluation);
    vertexShader.compile();
    fragmentShader.compile();
    tescShader.compile();
    teseShader.compile();
    terrainShader = ShaderProgram(vertexShader, fragmentShader,
                                  GeometryShader(), {tescShader, teseShader});
    terrainShader.compile();

    int width, height, nChannels;
    unsigned char *data = nullptr;
    std::vector<uint8_t> heightData = {};
    if (createdWithMap) {
        if (heightmap.type != ResourceType::Image) {
            throw std::runtime_error("Heightmap resource is not an image");
        }

        stbi_set_flip_vertically_on_load(false);
        data = stbi_load(heightmap.path.string().c_str(), &width, &height,
                         &nChannels, 0);
        if (data == nullptr) {
            std::cerr << "Failed to load heightmap!" << std::endl;
            return;
        }
    } else if (!createdWithMap && generator != nullptr) {
        width = this->width;
        height = this->height;
        heightData.resize(width * height * 4);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float heightValue = generator->generateHeight(x, y);
                uint8_t heightByte = static_cast<uint8_t>(
                    glm::clamp(heightValue * 255.f, 0.0f, 255.0f));

                int index = (y * width + x) * 4;
                heightData[index + 0] = heightByte; // R
                heightData[index + 1] = heightByte; // G
                heightData[index + 2] = heightByte; // B
                heightData[index + 3] = 255;        // A
            }
        }
        data = heightData.data();
        nChannels = 4;
    } else {
        throw std::runtime_error(
            "No heightmap resource or terrain generator provided");
    }

    this->generateBiomes(data, height, width, nChannels);

    glGenTextures(1, &terrainTexture.id);
    glBindTexture(GL_TEXTURE_2D, terrainTexture.id);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 nChannels == 4 ? GL_RGBA : (nChannels == 3 ? GL_RGB : GL_R16F),
                 width, height, 0,
                 nChannels == 4 ? GL_RGBA : (nChannels == 3 ? GL_RGB : GL_R16F),
                 GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    rez = resolution;

    for (unsigned int i = 0; i <= rez - 1; i++) {
        for (unsigned int j = 0; j <= rez - 1; j++) {
            vertices.push_back(-width / 2.0f + width * i / (float)rez);   // v.x
            vertices.push_back(0.0f);                                     // v.y
            vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
            vertices.push_back(i / (float)rez);                           // u
            vertices.push_back(j / (float)rez);                           // v

            vertices.push_back(-width / 2.0f +
                               width * (i + 1) / (float)rez);             // v.x
            vertices.push_back(0.0f);                                     // v.y
            vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
            vertices.push_back((i + 1) / (float)rez);                     // u
            vertices.push_back(j / (float)rez);                           // v

            vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
            vertices.push_back(0.0f);                                   // v.y
            vertices.push_back(-height / 2.0f +
                               height * (j + 1) / (float)rez); // v.z
            vertices.push_back(i / (float)rez);                // u
            vertices.push_back((j + 1) / (float)rez);          // v

            vertices.push_back(-width / 2.0f +
                               width * (i + 1) / (float)rez); // v.x
            vertices.push_back(0.0f);                         // v.y
            vertices.push_back(-height / 2.0f +
                               height * (j + 1) / (float)rez); // v.z
            vertices.push_back((i + 1) / (float)rez);          // u
            vertices.push_back((j + 1) / (float)rez);          // v
        }
    }

    if (generator == nullptr) {
        stbi_image_free(data);
    } else {
        data = nullptr;
    }

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    patch_count = 4;
    glPatchParameteri(GL_PATCH_VERTICES, patch_count);

    glBindVertexArray(0);
}

void Terrain::render(float dt) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glBindVertexArray(this->vao);
    glUseProgram(terrainShader.programId);

    terrainShader.setUniformMat4f("model", model);
    terrainShader.setUniformMat4f("view", view);
    terrainShader.setUniformMat4f("projection", projection);

    terrainShader.setUniform1f("maxPeak", maxPeak);
    terrainShader.setUniform1f("seaLevel", seaLevel);
    terrainShader.setUniform1i("isFromMap", createdWithMap ? 1 : 0);

    terrainShader.setUniform1i("heightMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainTexture.id);
    terrainShader.setUniform1i("moistureMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, moistureMapTexture.id);
    terrainShader.setUniform1i("temperatureMap", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, temperatureMapTexture.id);

    for (int i = 0; i < 12; i++) {
        terrainShader.setUniform1i("texture" + std::to_string(i), i + 4);
    }

    for (int i = 0; i < biomes.size(); i++) {
        Biome &biome = biomes[i];
        if (biome.useTexture) {
            terrainShader.setUniform1i(
                "biomes[" + std::to_string(i) + "].useTexture", 1);
            terrainShader.setUniform1i(
                "biomes[" + std::to_string(i) + "].textureId", i + 4);
            glActiveTexture(GL_TEXTURE3 + i);
            glBindTexture(GL_TEXTURE_2D, biome.texture.id);
        } else {
            terrainShader.setUniform1i(
                "biomes[" + std::to_string(i) + "].useTexture", 0);
        }
        std::string baseName = "biomes[" + std::to_string(i) + "]";
        terrainShader.setUniform1i(baseName + ".id", i);
        terrainShader.setUniform4f(baseName + ".tintColor", biomes[i].color.r,
                                   biomes[i].color.g, biomes[i].color.b,
                                   biomes[i].color.a);
        terrainShader.setUniform1f(baseName + ".minHeight",
                                   biomes[i].minHeight);
        terrainShader.setUniform1f(baseName + ".maxHeight",
                                   biomes[i].maxHeight);
        terrainShader.setUniform1f(baseName + ".minMoisture",
                                   biomes[i].minMoisture);
        terrainShader.setUniform1f(baseName + ".maxMoisture",
                                   biomes[i].maxMoisture);
        terrainShader.setUniform1f(baseName + ".minTemperature",
                                   biomes[i].minTemperature);
        terrainShader.setUniform1f(baseName + ".maxTemperature",
                                   biomes[i].maxTemperature);
    }
    terrainShader.setUniform1i("biomesCount", biomes.size());
    bool hasShadow = false;
    Window *mainWindow = Window::mainWindow;
    for (auto dirLight : mainWindow->getCurrentScene()->directionalLights) {
        terrainShader.setUniform3f("lightDir", dirLight->direction.x,
                                   dirLight->direction.y,
                                   dirLight->direction.z);
        terrainShader.setUniform4f("directionalColor", dirLight->color.r,
                                   dirLight->color.g, dirLight->color.b,
                                   dirLight->color.a);
        terrainShader.setUniform1f("directionalIntensity", dirLight->color.a);
        if (!dirLight->doesCastShadows)
            continue;
        hasShadow = true;
        terrainShader.setUniform1i("shadowMap", 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, dirLight->shadowRenderTarget->texture.id);
        ShadowParams shadowParams =
            dirLight->calculateLightSpaceMatrix(mainWindow->renderables);
        terrainShader.setUniformMat4f("lightViewProj",
                                      shadowParams.lightProjection *
                                          shadowParams.lightView);
        terrainShader.setUniform1f("shadowBias", shadowParams.bias);
    }

    if (mainWindow->getCurrentScene()->directionalLights.size() > 0) {
        terrainShader.setUniform1i("hasLight", 1);
    } else {
        terrainShader.setUniform1i("hasLight", 0);
    }

    if (!hasShadow) {
        terrainShader.setUniform1i("useShadowMap", 0);
    } else {
        terrainShader.setUniform1i("useShadowMap", 1);
    }

    Camera *camera = mainWindow->getCamera();
    terrainShader.setUniform3f("viewDir", camera->getFrontVector().x,
                               camera->getFrontVector().y,
                               camera->getFrontVector().z);
    AmbientLight ambient = mainWindow->getCurrentScene()->ambientLight;
    terrainShader.setUniform1f("ambientStrength", ambient.intensity * 4.0);

    glDrawArrays(GL_PATCHES, 0, patch_count * rez * rez);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void Terrain::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.roll)),
                    glm::vec3(0, 0, 1));
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.pitch)),
                    glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(
        rotation_matrix, glm::radians(float(rotation.yaw)), glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    this->model = translation_matrix * rotation_matrix * scale_matrix;
}