//
// terrain.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Main terrain defintion and functions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef AURORA_TERRAIN_H
#define AURORA_TERRAIN_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include "aurora/procedural.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Biome;

typedef unsigned int BufferIndex;
typedef std::function<void(Biome &)> BiomeFunction;

class Biome {
  public:
    std::string name;
    Texture texture;
    Color color = {1.0, 1.0, 1.0, 1.0};

    bool useTexture = false;

    void attachTexture(const Texture &tex) {
        this->texture = tex;
        this->useTexture = true;
    }

    float minHeight = -1.0f;
    float maxHeight = -1.0f;
    float minMoisture = -1.0f;
    float maxMoisture = -1.0f;
    float minTemperature = -1.0f;
    float maxTemperature = -1.0f;

    Biome(const std::string &biomeName = "",
          const Texture &biomeTexture = Texture(),
          const Color &biomeColor = Color())
        : name(biomeName), texture(biomeTexture), color(biomeColor) {}

    Biome(const std::string &biomeName = "", const Color &biomeColor = Color())
        : name(biomeName), color(biomeColor) {}

    BiomeFunction condition = [](Biome &thisBiome) { return; };
};

class Terrain : public GameObject {
  public:
    Resource heightmap;
    Texture moistureTexture;
    Texture temperatureTexture;
    std::shared_ptr<TerrainGenerator> generator;

    bool createdWithMap = false;

    int width = 0;
    int height = 0;

    void render(float dt) override;
    void initialize() override;

    bool usesDeferredRendering = true;
    bool canUseDeferredRendering() const override { return false; }

    void setViewMatrix(const glm::mat4 &view) override { this->view = view; };
    void setProjectionMatrix(const glm::mat4 &projection) override {
        this->projection = projection;
    };

    void addBiome(const Biome &biome) { this->biomes.push_back(biome); };

    Terrain(Resource heightmapResource)
        : heightmap(heightmapResource), createdWithMap(true) {};
    template <typename T>
        requires std::is_base_of<TerrainGenerator, T>::value
    Terrain(T generator, int width = 512, int height = 512)
        : generator(std::make_shared<T>(generator)), width(width),
          height(height), createdWithMap(false){};
    Terrain() = default;

    unsigned int resolution = 20;

    Position3d position;
    Rotation3d rotation;
    Scale3d scale = {1.0, 1.0, 1.0};

    std::vector<Biome> biomes;

    void setPosition(const Position3d &newPosition) override {
        this->position = newPosition;
    };

    void setRotation(const Rotation3d &newRotation) override {
        this->rotation = newRotation;
    };

    void setScale(const Scale3d &newScale) override { this->scale = newScale; };

    void move(const Position3d &deltaPosition) override {
        this->position.x += deltaPosition.x;
        this->position.y += deltaPosition.y;
        this->position.z += deltaPosition.z;
    };

    void updateModelMatrix();

    void scaleBy(const Scale3d &deltaScale) {
        this->scale.x *= deltaScale.x;
        this->scale.y *= deltaScale.y;
        this->scale.z *= deltaScale.z;
    };

    void rotate(const Rotation3d &deltaRotation) override {
        this->rotation.pitch += deltaRotation.pitch;
        this->rotation.yaw += deltaRotation.yaw;
        this->rotation.roll += deltaRotation.roll;
    };

    float maxPeak = 48.f;
    float seaLevel = 16.f;

  private:
    BufferIndex vao = 0;
    BufferIndex vbo = 0;
    BufferIndex ebo = 0;
    ShaderProgram terrainShader;

    Texture terrainTexture;
    Texture biomesTexture;
    Texture moistureMapTexture;
    Texture temperatureMapTexture;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    std::vector<float> vertices;
    std::vector<uint8_t> moistureData;
    std::vector<uint8_t> temperatureData;

    unsigned int patch_count;
    unsigned int rez;

    void generateBiomes(unsigned char *heightmapData, int width, int height,
                        int nChannels);
    void generateMaps(unsigned char *heightmapData, int width, int height,
                      int generationParameter, int nChannels);
};

namespace aurora {
float computeSlope(const uint8_t *heightMap, int width, int height, int x,
                   int y);
}

#endif // AURORA_TERRAIN_H