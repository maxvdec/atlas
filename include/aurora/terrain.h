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
/**
 * @brief Callback that configures runtime biome properties such as textures
 * or thresholds.
 */
typedef std::function<void(Biome &)> BiomeFunction;

/**
 * @brief Describes the visual and environmental parameters for a terrain
 * region.
 *
 * \subsection biome-example Example
 * ```cpp
 * Biome grassland("Grassland", Color(0.3, 0.6, 0.2, 1.0));
 * grassland.minHeight = 0.0f;
 * grassland.maxHeight = 20.0f;
 * grassland.condition = [](Biome &b) {
 *     b.attachTexture(Texture::fromResourceName("Grass"));
 * };
 * ```
 */
class Biome {
  public:
    /**
     * @brief Human readable name used for identification.
     */
    std::string name;
    /**
     * @brief Texture applied when `useTexture` is enabled.
     */
    Texture texture;
    /**
     * @brief Fallback tint applied when no texture is used.
     */
    Color color = {1.0, 1.0, 1.0, 1.0};

    /**
     * @brief Whether the biome should render the assigned texture.
     */
    bool useTexture = false;

    /**
     * @brief Assigns a texture and enables textured rendering for the biome.
     *
     * @param tex Texture resource to sample.
     */
    void attachTexture(const Texture &tex) {
        this->texture = tex;
        this->useTexture = true;
    }

    /**
     * @brief Minimum height threshold for this biome to appear.
     */
    float minHeight = -1.0f;
    /**
     * @brief Maximum height threshold for this biome to appear.
     */
    float maxHeight = -1.0f;
    /**
     * @brief Minimum moisture value required for this biome.
     */
    float minMoisture = -1.0f;
    /**
     * @brief Maximum moisture value allowed for this biome.
     */
    float maxMoisture = -1.0f;
    /**
     * @brief Minimum temperature threshold.
     */
    float minTemperature = -1.0f;
    /**
     * @brief Maximum temperature threshold.
     */
    float maxTemperature = -1.0f;

    /**
     * @brief Creates a biome that uses both a texture and color tint.
     *
     * @param biomeName Name assigned to the biome.
     * @param biomeTexture Texture used when sampling.
     * @param biomeColor Fallback color tint.
     */
    Biome(const std::string &biomeName = "",
          const Texture &biomeTexture = Texture(),
          const Color &biomeColor = Color())
        : name(biomeName), texture(biomeTexture), color(biomeColor) {}

    /**
     * @brief Creates a color-only biome configuration.
     *
     * @param biomeName Name assigned to the biome.
     * @param biomeColor Color tint to apply.
     */
    Biome(const std::string &biomeName = "", const Color &biomeColor = Color())
        : name(biomeName), color(biomeColor) {}

    /**
     * @brief Optional hook executed before the biome is rasterized.
     */
    BiomeFunction condition = [](Biome &) { return; };
};

/**
 * @brief Tessellated terrain surface that can be generated procedurally or
 * from heightmaps.
 *
 * \subsection terrain-example Example
 * ```cpp
 * Terrain terrain(HillGenerator());
 * terrain.width = 512;
 * terrain.height = 512;
 * terrain.initialize();
 * terrain.addBiome(Biome("Snow", Color::white()));
 * window.addObject(&terrain);
 * ```
 */
class Terrain : public GameObject {
  public:
    /**
     * @brief Heightmap resource used when the terrain is loaded from disk.
     */
    Resource heightmap;
    /**
     * @brief Procedurally generated moisture data encoded as a texture.
     */
    Texture moistureTexture;
    /**
     * @brief Procedurally generated temperature data encoded as a texture.
     */
    Texture temperatureTexture;
    /**
     * @brief Active terrain height generator when not using a heightmap.
     */
    std::shared_ptr<TerrainGenerator> generator;

    /**
     * @brief Indicates whether this terrain originated from a heightmap file.
     */
    bool createdWithMap = false;

    /**
     * @brief Number of vertices along the X axis.
     */
    int width = 0;
    /**
     * @brief Number of vertices along the Z axis.
     */
    int height = 0;

    /**
     * @brief Renders the terrain patches.
     */
    void render(float dt, bool updatePipeline = false) override;
    /**
     * @brief Builds buffers, materials, and generator data.
     */
    void initialize() override;

    /**
     * @brief Indicates if the renderer should attempt deferred shading paths.
     */
    bool usesDeferredRendering = true;
    /**
     * @brief Terrains render forward-only regardless of the window setting.
     */
    bool canUseDeferredRendering() override { return false; }

    /**
     * @brief Stores the view matrix used for terrain rendering.
     */
    void setViewMatrix(const glm::mat4 &view) override { this->view = view; };
    /**
     * @brief Stores the projection matrix used for terrain rendering.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override {
        this->projection = projection;
    };

    /**
     * @brief Registers a biome definition for later map generation.
     */
    void addBiome(const Biome &biome) { this->biomes.push_back(biome); };

    /**
     * @brief Creates a terrain backed by a heightmap resource.
     *
     * @param heightmapResource Resource pointing to the heightmap image.
     */
    Terrain(Resource heightmapResource)
        : heightmap(heightmapResource), createdWithMap(true) {};
    /**
     * @brief Creates a procedurally generated terrain.
     *
     * @param generator Height generator instance used to synthesize data.
     * @param width Width of the generated grid in vertices.
     * @param height Height of the generated grid in vertices.
     */
    template <typename T>
        requires std::is_base_of<TerrainGenerator, T>::value
    Terrain(T generator, int width = 512, int height = 512)
        : generator(std::make_shared<T>(generator)), createdWithMap(false),
          width(width), height(height){};
    Terrain() = default;

    /**
     * @brief LOD resolution multiplier controlling tessellation density.
     */
    unsigned int resolution = 20;

    /**
     * @brief Terrain origin relative to world space.
     */
    Position3d position;
    /**
     * @brief Euler rotation applied to the terrain.
     */
    Rotation3d rotation;
    /**
     * @brief Scale applied across each axis.
     */
    Scale3d scale = {1.0, 1.0, 1.0};

    /**
     * @brief Collection of biomes available for classification.
     */
    std::vector<Biome> biomes;

    /**
     * @brief Moves the terrain to an absolute world-space position.
     */
    void setPosition(const Position3d &newPosition) override {
        this->position = newPosition;
    };

    /**
     * @brief Sets the terrain rotation in Euler angles.
     */
    void setRotation(const Rotation3d &newRotation) override {
        this->rotation = newRotation;
    };

    /**
     * @brief Sets the absolute terrain scale.
     */
    void setScale(const Scale3d &newScale) override { this->scale = newScale; };

    /**
     * @brief Translates the terrain by the given delta.
     */
    void move(const Position3d &deltaPosition) override {
        this->position.x += deltaPosition.x;
        this->position.y += deltaPosition.y;
        this->position.z += deltaPosition.z;
    };

    /**
     * @brief Recomputes the model matrix from the current transform.
     */
    void updateModelMatrix();

    /**
     * @brief Scales the terrain relative to its current size.
     */
    void scaleBy(const Scale3d &deltaScale) {
        this->scale.x *= deltaScale.x;
        this->scale.y *= deltaScale.y;
        this->scale.z *= deltaScale.z;
    };

    /**
     * @brief Rotates the terrain incrementally along each axis.
     */
    void rotate(const Rotation3d &deltaRotation) override {
        this->rotation.pitch += deltaRotation.pitch;
        this->rotation.yaw += deltaRotation.yaw;
        this->rotation.roll += deltaRotation.roll;
    };

    /**
     * @brief Maximum elevation (in world units) used when normalizing heights.
     */
    float maxPeak = 48.f;
    /**
     * @brief Elevation at which water begins to cover the terrain.
     */
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
/**
 * @brief Computes the slope factor at a point on a heightmap.
 *
 * @param heightMap Pointer to height data.
 * @param width Width of the heightmap.
 * @param height Height of the heightmap.
 * @param x Sample X coordinate.
 * @param y Sample Y coordinate.
 * @return Gradient magnitude representing slope steepness.
 */
float computeSlope(const uint8_t *heightMap, int width, int height, int x,
                   int y);
} // namespace aurora

#endif // AURORA_TERRAIN_H