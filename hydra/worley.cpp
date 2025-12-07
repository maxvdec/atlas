/*
 worley.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Worley noise generation utilities
 Copyright (c) 2025 Max Van den Eynde
*/

#include <hydra/atmosphere.h>

#include <opal/opal.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

namespace {
constexpr float SQRT3 = 1.7320508075688772f;

inline uint32_t hash3d(int x, int y, int z, uint32_t seed) {
    uint32_t h = seed;
    h ^= static_cast<uint32_t>(x) * 0x6C8E9CF5u;
    h ^= static_cast<uint32_t>(y) * 0x5D588B65u;
    h ^= static_cast<uint32_t>(z) * 0x7F4A7C15u;
    h ^= (h >> 13u);
    h *= 0x85EBCA6Bu;
    h ^= (h >> 16u);
    return h;
}

inline float randomFloat(uint32_t &state) {
    state ^= state << 13u;
    state ^= state >> 17u;
    state ^= state << 5u;
    return static_cast<float>(state & 0xFFFFFFu) /
           static_cast<float>(0xFFFFFFu);
}

inline float wrapFloat(float value) {
    float wrapped = std::fmod(value, 1.0f);
    if (wrapped < 0.0f) {
        wrapped += 1.0f;
    }
    return wrapped;
}

template <typename Func> void parallelForRange(int begin, int end, Func func) {
    const int total = end - begin;
    if (total <= 0) {
        return;
    }

    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads == 0) {
        hardwareThreads = 4;
    }

    unsigned int threadCount = std::max(
        1u, std::min(hardwareThreads, static_cast<unsigned int>(total)));
    const int chunkSize = (total + static_cast<int>(threadCount) - 1) /
                          static_cast<int>(threadCount);

    std::vector<std::thread> workers;
    workers.reserve(threadCount > 0 ? threadCount - 1 : 0);

    int next = begin;
    for (unsigned int i = 0; i + 1 < threadCount && next < end; ++i) {
        int blockStart = next;
        int blockEnd = std::min(end, blockStart + chunkSize);
        workers.emplace_back(
            [blockStart, blockEnd, &func]() { func(blockStart, blockEnd); });
        next = blockEnd;
    }

    if (next < end) {
        func(next, end);
    }

    for (auto &worker : workers) {
        worker.join();
    }
}
} // namespace

WorleyNoise3D::WorleyNoise3D(int frequency, int numberOfDivisions)
    : frequency(std::max(1, frequency)),
      numberOfDivisions(std::max(1, numberOfDivisions)) {
    generateFeaturePoints();
}

float WorleyNoise3D::getValue(float x, float y, float z) const {
    if (frequency <= 0) {
        return 0.0f;
    }

    float amplitude = 1.0f;
    float sum = 0.0f;
    float normalization = 0.0f;

    for (int octave = 0; octave < frequency; ++octave) {
        float value = getWorleyNoise(x, y, z, octave);
        sum += value * amplitude;
        normalization += amplitude;
        amplitude *= 0.5f;
    }

    if (normalization <= 0.0f) {
        return 0.0f;
    }

    float averaged = sum / normalization;
    return std::clamp(averaged, 0.0f, 1.0f);
}

Id WorleyNoise3D::get3dTexture(int res) const {
    int resolution = std::max(1, res);
    constexpr size_t channelCount = 4;
    std::vector<float> data(static_cast<size_t>(resolution) * resolution *
                                resolution * channelCount,
                            0.0f);

    const float invResolution = 1.0f / static_cast<float>(resolution);

    parallelForRange(0, resolution, [&](int zStart, int zEnd) {
        for (int z = zStart; z < zEnd; ++z) {
            size_t sliceIndex =
                static_cast<size_t>(z) * resolution * resolution * channelCount;
            for (int y = 0; y < resolution; ++y) {
                size_t rowIndex = sliceIndex + static_cast<size_t>(y) *
                                                   resolution * channelCount;
                for (int x = 0; x < resolution; ++x) {
                    float fx = (static_cast<float>(x) + 0.5f) * invResolution;
                    float fy = (static_cast<float>(y) + 0.5f) * invResolution;
                    float fz = (static_cast<float>(z) + 0.5f) * invResolution;

                    float low = getWorleyNoise(fx, fy, fz, 0);
                    float mid = getWorleyNoise(fx, fy, fz, 1);
                    float high = getWorleyNoise(fx, fy, fz, 2);
                    float combined = getValue(fx, fy, fz);

                    size_t index =
                        rowIndex + static_cast<size_t>(x) * channelCount;
                    data[index + 0] = low;
                    data[index + 1] = mid;
                    data[index + 2] = high;
                    data[index + 3] = combined;
                }
            }
        }
    });

    return createTexture3d(data, resolution);
}

Id WorleyNoise3D::getDetailTexture(int res) const {
    int resolution = std::max(1, res);
    constexpr size_t channelCount = 4;
    std::vector<float> data(static_cast<size_t>(resolution) * resolution *
                                resolution * channelCount,
                            0.0f);

    const float invResolution = 1.0f / static_cast<float>(resolution);

    parallelForRange(0, resolution, [&](int zStart, int zEnd) {
        for (int z = zStart; z < zEnd; ++z) {
            size_t sliceIndex =
                static_cast<size_t>(z) * resolution * resolution * channelCount;
            for (int y = 0; y < resolution; ++y) {
                size_t rowIndex = sliceIndex + static_cast<size_t>(y) *
                                                   resolution * channelCount;
                for (int x = 0; x < resolution; ++x) {
                    float fx = (static_cast<float>(x) + 0.5f) * invResolution;
                    float fy = (static_cast<float>(y) + 0.5f) * invResolution;
                    float fz = (static_cast<float>(z) + 0.5f) * invResolution;

                    auto distances = getClosestDistances(
                        fx * numberOfDivisions, fy * numberOfDivisions,
                        fz * numberOfDivisions, 3);

                    float f1 = distances.size() > 0 ? distances[0] : 0.0f;
                    float f2 = distances.size() > 1 ? distances[1] : f1;
                    float f3 = distances.size() > 2 ? distances[2] : f2;

                    float difference =
                        std::clamp((f2 - f1) / SQRT3, 0.0f, 1.0f);
                    float ridge = 1.0f - f1 / SQRT3;
                    float turbulence =
                        std::clamp((f3 - f1) / SQRT3, 0.0f, 1.0f);

                    size_t index =
                        rowIndex + static_cast<size_t>(x) * channelCount;
                    data[index + 0] = difference;
                    data[index + 1] = ridge;
                    data[index + 2] = turbulence;
                    data[index + 3] = 1.0f;
                }
            }
        }
    });

    return createTexture3d(data, resolution);
}

Id WorleyNoise3D::get3dTextureAtAllChannels(int res) const {
    int resolution = std::max(1, res);
    constexpr size_t channelCount = 4;
    std::vector<float> data(static_cast<size_t>(resolution) * resolution *
                                resolution * channelCount,
                            0.0f);

    const float invResolution = 1.0f / static_cast<float>(resolution);

    parallelForRange(0, resolution, [&](int zStart, int zEnd) {
        for (int z = zStart; z < zEnd; ++z) {
            size_t sliceIndex =
                static_cast<size_t>(z) * resolution * resolution * channelCount;
            for (int y = 0; y < resolution; ++y) {
                size_t rowIndex = sliceIndex + static_cast<size_t>(y) *
                                                   resolution * channelCount;
                for (int x = 0; x < resolution; ++x) {
                    float fx = (static_cast<float>(x) + 0.5f) * invResolution;
                    float fy = (static_cast<float>(y) + 0.5f) * invResolution;
                    float fz = (static_cast<float>(z) + 0.5f) * invResolution;
                    float value = getValue(fx, fy, fz);

                    size_t index =
                        rowIndex + static_cast<size_t>(x) * channelCount;
                    data[index + 0] = value;
                    data[index + 1] = value;
                    data[index + 2] = value;
                    data[index + 3] = value;
                }
            }
        }
    });

    return createTexture3d(data, resolution);
}

void WorleyNoise3D::generateFeaturePoints() {
    size_t cells = static_cast<size_t>(numberOfDivisions) * numberOfDivisions *
                   numberOfDivisions;
    featurePoints.resize(cells * static_cast<size_t>(frequency));

    for (int z = 0; z < numberOfDivisions; ++z) {
        for (int y = 0; y < numberOfDivisions; ++y) {
            for (int x = 0; x < numberOfDivisions; ++x) {
                uint32_t baseHash = hash3d(x, y, z, 0x1F123BB5u);
                size_t cellIndex =
                    (static_cast<size_t>(z) * numberOfDivisions *
                     numberOfDivisions) +
                    (static_cast<size_t>(y) * numberOfDivisions) +
                    static_cast<size_t>(x);
                size_t featureBase = cellIndex * static_cast<size_t>(frequency);

                for (int i = 0; i < frequency; ++i) {
                    uint32_t state =
                        baseHash ^ (0x9E3779B9u * static_cast<uint32_t>(i + 1));
                    float rx = randomFloat(state);
                    float ry = randomFloat(state);
                    float rz = randomFloat(state);

                    featurePoints[featureBase + static_cast<size_t>(i)] =
                        glm::vec3(static_cast<float>(x) + rx,
                                  static_cast<float>(y) + ry,
                                  static_cast<float>(z) + rz);
                }
            }
        }
    }
}

float WorleyNoise3D::getWorleyNoise(float x, float y, float z,
                                    int octave) const {
    if (numberOfDivisions <= 0) {
        return 0.0f;
    }

    int level = std::max(0, octave);
    float scale = std::ldexp(1.0f, level);
    float div = static_cast<float>(numberOfDivisions);

    glm::vec3 scaled = glm::vec3(x, y, z) * scale * div;
    auto distances = getClosestDistances(scaled.x, scaled.y, scaled.z, 1);

    if (distances.empty()) {
        return 0.0f;
    }

    float normalized = 1.0f - std::clamp(distances[0] / SQRT3, 0.0f, 1.0f);
    return normalized;
}

std::vector<float> WorleyNoise3D::getClosestDistances(float x, float y, float z,
                                                      int count) const {
    if (featurePoints.empty() || count <= 0) {
        return {};
    }

    const int sampleCount = std::min(count, frequency * 27);
    std::vector<float> distances(static_cast<size_t>(sampleCount),
                                 std::numeric_limits<float>::max());

    glm::vec3 p(x, y, z);
    glm::ivec3 baseCell(static_cast<int>(std::floor(p.x)),
                        static_cast<int>(std::floor(p.y)),
                        static_cast<int>(std::floor(p.z)));

    auto wrapIndex = [this](int value) {
        int wrapped = value % numberOfDivisions;
        return wrapped < 0 ? wrapped + numberOfDivisions : wrapped;
    };

    for (int dz = -1; dz <= 1; ++dz) {
        int cellZ = baseCell.z + dz;
        int wrappedZ = wrapIndex(cellZ);
        int offsetZ = cellZ - wrappedZ;

        for (int dy = -1; dy <= 1; ++dy) {
            int cellY = baseCell.y + dy;
            int wrappedY = wrapIndex(cellY);
            int offsetY = cellY - wrappedY;

            for (int dx = -1; dx <= 1; ++dx) {
                int cellX = baseCell.x + dx;
                int wrappedX = wrapIndex(cellX);
                int offsetX = cellX - wrappedX;

                int baseIndex = getCellIndex(wrappedX, wrappedY, wrappedZ);
                size_t featureStart = static_cast<size_t>(baseIndex) *
                                      static_cast<size_t>(frequency);

                for (int i = 0; i < frequency; ++i) {
                    const glm::vec3 &feature =
                        featurePoints[featureStart + static_cast<size_t>(i)];
                    glm::vec3 featurePos =
                        feature + glm::vec3(static_cast<float>(offsetX),
                                            static_cast<float>(offsetY),
                                            static_cast<float>(offsetZ));
                    glm::vec3 diff = featurePos - p;
                    float distSq = glm::dot(diff, diff);

                    auto worstIt =
                        std::max_element(distances.begin(), distances.end());
                    if (distSq < *worstIt) {
                        *worstIt = distSq;
                    }
                }
            }
        }
    }

    for (float &d : distances) {
        d = std::sqrt(std::max(d, 0.0f));
    }

    std::sort(distances.begin(), distances.end());
    return distances;
}

glm::ivec3 WorleyNoise3D::getGridCell(float x, float y, float z) const {
    glm::vec3 p(wrapFloat(x), wrapFloat(y), wrapFloat(z));
    glm::vec3 scaled = p * static_cast<float>(numberOfDivisions);
    return glm::ivec3(static_cast<int>(std::floor(scaled.x)),
                      static_cast<int>(std::floor(scaled.y)),
                      static_cast<int>(std::floor(scaled.z)));
}

int WorleyNoise3D::getCellIndex(int cx, int cy, int cz) const {
    int wrappedX =
        (cx % numberOfDivisions + numberOfDivisions) % numberOfDivisions;
    int wrappedY =
        (cy % numberOfDivisions + numberOfDivisions) % numberOfDivisions;
    int wrappedZ =
        (cz % numberOfDivisions + numberOfDivisions) % numberOfDivisions;

    return ((wrappedZ * numberOfDivisions) + wrappedY) * numberOfDivisions +
           wrappedX;
}

Id WorleyNoise3D::createTexture3d(const std::vector<float> &data,
                                  int res) const {
    if (data.empty() || res <= 0) {
        return 0;
    }

    // Use opal to create the 3D texture
    auto texture =
        opal::Texture::create3D(opal::TextureFormat::Rgba16F, res, res, res,
                                opal::TextureDataFormat::Rgba, data.data());

    if (!texture) {
        return 0;
    }

    // Set texture parameters using opal
    texture->setParameters3D(
        opal::TextureWrapMode::Repeat, opal::TextureWrapMode::Repeat,
        opal::TextureWrapMode::Repeat, opal::TextureFilterMode::Linear,
        opal::TextureFilterMode::Linear);

    return texture->textureID;
}
