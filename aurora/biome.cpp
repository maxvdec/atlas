//
// biome.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Biome generation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/texture.h"
#include "aurora/terrain.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <vector>
#include <glad/glad.h>

float aurora::computeSlope(const uint8_t *heightMap, int width, int height,
                           int x, int y) {
    int xm = std::max(x - 1, 0);
    int xp = std::min(x + 1, width - 1);
    int ym = std::max(y - 1, 0);
    int yp = std::min(y + 1, height - 1);

    float dzdx =
        float(heightMap[xp + y * width]) - float(heightMap[xm + y * width]);
    float dzdy =
        float(heightMap[x + yp * width]) - float(heightMap[x + ym * width]);

    return std::sqrt(dzdx * dzdx + dzdy * dzdy) / 255.0f;
}

void Terrain::generateMaps(unsigned char *heightmapData, int height, int width,
                           int generationParameter, int nChannels) {
    const float waterLevel = 64.0f;
    const float maxHeight = 255.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = x + y * width;

            uint8_t heightValue;
            if (nChannels == 1) {
                heightValue = heightmapData[idx];
            } else if (nChannels >= 3) {
                int r = heightmapData[idx * nChannels + 0];
                int g = heightmapData[idx * nChannels + 1];
                int b = heightmapData[idx * nChannels + 2];
                heightValue = static_cast<uint8_t>((r + g + b) / 3);
            } else {
                heightValue = 0;
            }

            if (generationParameter == 1) {
                float temperature = std::max(
                    0.0f, 1.0f - (float(heightValue) / float(maxHeight)));
                uint8_t temperatureValue =
                    static_cast<uint8_t>(temperature * 255.0f);
                this->temperatureData.push_back(temperatureValue);
            }

            if (generationParameter == 2) {
                float slope =
                    aurora::computeSlope(heightmapData, width, height, x, y);
                float moisture = (1.0f - slope) *
                                 (1.0f - float(heightValue) / float(maxHeight));
                moisture += ((rand() % 100) / 1000.0f) * 0.2; // Add some noise
                moisture = std::clamp(moisture, 0.0f, 1.0f);
                uint8_t moistureValue = static_cast<uint8_t>(moisture * 255.0f);

                this->moistureData.push_back(moistureValue);
            }
        }
    }
}

void Terrain::generateBiomes(unsigned char *heightmapData, int height,
                             int width, int nChannels) {
    if (moistureTexture.id == 0) {
        generateMaps(heightmapData, width, height, 2, nChannels);
    } else {
        std::vector<uint8_t> data(width * height * 4);
        glBindTexture(GL_TEXTURE_2D, moistureTexture.id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        for (int i = 0; i < width * height; ++i) {
            this->moistureData.push_back(data[i * 4]);
        }
    }

    if (temperatureTexture.id == 0) {
        generateMaps(heightmapData, width, height, 1, nChannels);
    } else {
        std::vector<uint8_t> data(width * height * 4);
        glBindTexture(GL_TEXTURE_2D, temperatureTexture.id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        for (int i = 0; i < width * height; ++i) {
            this->temperatureData.push_back(data[i * 4]);
        }
    }

    if (biomesTexture.id != 0) {
        return;
    }

    glGenTextures(1, &biomesTexture.id);
    glBindTexture(GL_TEXTURE_2D, biomesTexture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    std::vector<uint8_t> biomeData(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = x + y * width;

            uint8_t heightValue;
            if (nChannels == 1) {
                heightValue = heightmapData[idx];
            } else if (nChannels >= 3) {
                int r = heightmapData[idx * nChannels + 0];
                int g = heightmapData[idx * nChannels + 1];
                int b = heightmapData[idx * nChannels + 2];
                heightValue = static_cast<uint8_t>((r + g + b) / 3);
            }

            uint8_t moisture = this->moistureData[idx];
            uint8_t temperature = this->temperatureData[idx];

            bool matched = false;
            for (size_t j = 0; j < biomes.size(); ++j) {
                const Biome &biome = biomes[j];
                if (biome.condition(heightValue, moisture, temperature)) {
                    biomeData[idx] =
                        static_cast<uint8_t>((j + 1) * 255 / biomes.size());
                    matched = true;
                    break;
                }
            }

            if (biomes.size() == 0) {
                matched = false;
            }

            if (!matched) {
                biomeData[idx] = 0;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, biomesTexture.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED,
                    GL_UNSIGNED_BYTE, biomeData.data());
}