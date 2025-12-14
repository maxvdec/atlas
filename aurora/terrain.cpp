//
// terrain.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Terrain functions implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <cstddef>
#include <cstdint>
#include "atlas/tracer/data.h"
#include "opal/opal.h"
#include "atlas/camera.h"
#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <aurora/terrain.h>
#include <iostream>
#include <vector>
#include "stb/stb_image.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Terrain::initialize() {
    atlas_log("Initializing terrain");
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
            atlas_error("Heightmap resource is not an image");
            throw std::runtime_error("Heightmap resource is not an image");
        }

        stbi_set_flip_vertically_on_load(false);
        data = stbi_load(heightmap.path.string().c_str(), &width, &height,
                         &nChannels, 0);
        if (data == nullptr) {
            atlas_error("Failed to load heightmap: " + heightmap.path.string());
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
        atlas_error("No heightmap resource or terrain generator provided");
        throw std::runtime_error(
            "No heightmap resource or terrain generator provided");
    }

    this->generateBiomes(data, height, width, nChannels);

    opal::TextureFormat texFormat =
        nChannels == 4 ? opal::TextureFormat::Rgba8
                       : (nChannels == 3 ? opal::TextureFormat::Rgb8
                                         : opal::TextureFormat::Rgb16F);
    opal::TextureDataFormat dataFormat =
        nChannels == 4 ? opal::TextureDataFormat::Rgba
                       : (nChannels == 3 ? opal::TextureDataFormat::Rgb
                                         : opal::TextureDataFormat::Red);
    terrainTexture.texture =
        opal::Texture::create(opal::TextureType::Texture2D, texFormat, width,
                              height, dataFormat, data, 1);
    terrainTexture.texture->setWrapMode(opal::TextureAxis::S,
                                        opal::TextureWrapMode::Repeat);
    terrainTexture.texture->setWrapMode(opal::TextureAxis::T,
                                        opal::TextureWrapMode::Repeat);
    terrainTexture.texture->setFilterMode(
        opal::TextureFilterMode::LinearMipmapLinear,
        opal::TextureFilterMode::Linear);
    terrainTexture.texture->automaticallyGenerateMipmaps();
    terrainTexture.id = terrainTexture.texture->textureID;

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

    vertexBuffer = opal::Buffer::create(
        opal::BufferUsage::VertexBuffer, vertices.size() * sizeof(float),
        vertices.data(), opal::MemoryUsageType::GPUOnly, id);

    drawingState = opal::DrawingState::create(vertexBuffer, nullptr);

    std::vector<opal::VertexAttributeBinding> attributeBindings = {
        {opal::VertexAttribute{.name = "position",
                               .type = opal::VertexAttributeType::Float,
                               .offset = 0,
                               .location = 0,
                               .normalized = false,
                               .size = 3,
                               .stride = 5 * sizeof(float)},
         vertexBuffer},
        {opal::VertexAttribute{.name = "texCoord",
                               .type = opal::VertexAttributeType::Float,
                               .offset = 3 * sizeof(float),
                               .location = 1,
                               .normalized = false,
                               .size = 2,
                               .stride = 5 * sizeof(float)},
         vertexBuffer}};
    drawingState->configureAttributes(attributeBindings);

    patch_count = 4;
}

void Terrain::render(float, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                     bool updatePipeline) {
    (void)updatePipeline;

    static std::shared_ptr<opal::Pipeline> terrainPipeline = nullptr;
    if (terrainPipeline == nullptr) {
        terrainPipeline = opal::Pipeline::create();
    }
    terrainPipeline = terrainShader.requestPipeline(terrainPipeline);

    terrainPipeline->enableDepthTest(true);
    terrainPipeline->setDepthCompareOp(opal::CompareOp::Less);
    terrainPipeline->enableDepthWrite(true);
    terrainPipeline->setCullMode(opal::CullMode::Back);
    terrainPipeline->setFrontFace(opal::FrontFace::Clockwise);
    terrainPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Patches);
    terrainPipeline->setPatchVertices(patch_count);
    terrainPipeline->bind();

    commandBuffer->bindDrawingState(drawingState);

    terrainPipeline->setUniformMat4f("model", model);
    terrainPipeline->setUniformMat4f("view", view);
    terrainPipeline->setUniformMat4f("projection", projection);

    terrainPipeline->setUniform1f("maxPeak", maxPeak);
    terrainPipeline->setUniform1f("seaLevel", seaLevel);
    terrainPipeline->setUniform1i("isFromMap", createdWithMap ? 1 : 0);

    terrainPipeline->bindTexture2D("heightMap", terrainTexture.id, 0, id);
    terrainPipeline->bindTexture2D("moistureMap", moistureMapTexture.id, 1, id);
    terrainPipeline->bindTexture2D("temperatureMap", temperatureMapTexture.id,
                                   2, id);

    for (int i = 0; i < 12; i++) {
        terrainPipeline->setUniform1i("texture" + std::to_string(i), i + 4);
    }

    for (size_t i = 0; i < biomes.size(); i++) {
        Biome &biome = biomes[i];
        if (biome.useTexture) {
            terrainPipeline->setUniform1i(
                "biomes[" + std::to_string(i) + "].useTexture", 1);
            terrainPipeline->setUniform1i(
                "biomes[" + std::to_string(i) + "].textureId", i + 4);
            terrainPipeline->bindTexture2D("biomeTexture" + std::to_string(i),
                                           biome.texture.id, 3 + i, id);
        } else {
            terrainPipeline->setUniform1i(
                "biomes[" + std::to_string(i) + "].useTexture", 0);
        }
        std::string baseName = "biomes[" + std::to_string(i) + "]";
        terrainPipeline->setUniform1i(baseName + ".id", i);
        terrainPipeline->setUniform4f(baseName + ".tintColor",
                                      biomes[i].color.r, biomes[i].color.g,
                                      biomes[i].color.b, biomes[i].color.a);
        terrainPipeline->setUniform1f(baseName + ".minHeight",
                                      biomes[i].minHeight);
        terrainPipeline->setUniform1f(baseName + ".maxHeight",
                                      biomes[i].maxHeight);
        terrainPipeline->setUniform1f(baseName + ".minMoisture",
                                      biomes[i].minMoisture);
        terrainPipeline->setUniform1f(baseName + ".maxMoisture",
                                      biomes[i].maxMoisture);
        terrainPipeline->setUniform1f(baseName + ".minTemperature",
                                      biomes[i].minTemperature);
        terrainPipeline->setUniform1f(baseName + ".maxTemperature",
                                      biomes[i].maxTemperature);
    }
    terrainPipeline->setUniform1i("biomesCount", biomes.size());
    bool hasShadow = false;
    Window *mainWindow = Window::mainWindow;
    for (auto dirLight : mainWindow->getCurrentScene()->directionalLights) {
        terrainPipeline->setUniform3f("lightDir", dirLight->direction.x,
                                      dirLight->direction.y,
                                      dirLight->direction.z);
        terrainPipeline->setUniform4f("directionalColor", dirLight->color.r,
                                      dirLight->color.g, dirLight->color.b,
                                      dirLight->color.a);
        terrainPipeline->setUniform1f("directionalIntensity",
                                      dirLight->color.a);
        if (!dirLight->doesCastShadows)
            continue;
        hasShadow = true;
        terrainPipeline->bindTexture2D(
            "shadowMap", dirLight->shadowRenderTarget->texture.id, 3, id);
        ShadowParams shadowParams =
            dirLight->calculateLightSpaceMatrix(mainWindow->renderables);
        terrainPipeline->setUniformMat4f("lightViewProj",
                                         shadowParams.lightProjection *
                                             shadowParams.lightView);
        terrainPipeline->setUniform1f("shadowBias", shadowParams.bias);
    }

    if (mainWindow->getCurrentScene()->directionalLights.size() > 0) {
        terrainPipeline->setUniform1i("hasLight", 1);
    } else {
        terrainPipeline->setUniform1i("hasLight", 0);
    }

    if (!hasShadow) {
        terrainPipeline->setUniform1i("useShadowMap", 0);
    } else {
        terrainPipeline->setUniform1i("useShadowMap", 1);
    }

    Camera *camera = mainWindow->getCamera();
    terrainPipeline->setUniform3f("viewDir", camera->getFrontVector().x,
                                  camera->getFrontVector().y,
                                  camera->getFrontVector().z);
    AmbientLight ambient = mainWindow->getCurrentScene()->ambientLight;
    terrainPipeline->setUniform1f("ambientStrength", ambient.intensity * 4.0);

    commandBuffer->drawPatches(patch_count * rez * rez, 0, id);
    commandBuffer->unbindDrawingState();

    terrainPipeline->setCullMode(opal::CullMode::Back);
    terrainPipeline->setFrontFace(opal::FrontFace::CounterClockwise);
    terrainPipeline->bind();

    DebugObjectPacket debugPacket{};
    debugPacket.drawCallsForObject = 1;
    debugPacket.triangleCount =
        static_cast<uint32_t>(patch_count * rez * rez) * 2;
    debugPacket.vertexBufferSizeMb =
        static_cast<float>(vertices.size() * sizeof(float)) /
        (1024.0f * 1024.0f);
    debugPacket.indexBufferSizeMb = 0.0f;
    debugPacket.textureCount = 3 + static_cast<uint32_t>(biomes.size());
    debugPacket.materialCount = 0;
    debugPacket.objectType = DebugObjectType::Terrain;
    debugPacket.objectId = this->id;

    debugPacket.send();
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