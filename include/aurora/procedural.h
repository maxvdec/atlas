//
// procedural.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Procedural Terrain Generation
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef AURORA_PROCEDURAL_H
#define AURORA_PROCEDURAL_H

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
struct PerlinNoise {
  private:
    static std::vector<int> p;

    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y);

    static std::vector<int> permutation;

  public:
    PerlinNoise(unsigned int seed = 0);

    float noise(float x, float y) const;
};

class SimplexNoise {
  private:
    static const int grad3[12][3];
    static int fastfloor(float x);
    static float dot(const int *g, float x, float y);

  public:
    static float noise(float xin, float yin);
};

class WorleyNoise {
  private:
    int numPoints;
    std::vector<std::pair<float, float>> featurePoints;

  public:
    WorleyNoise(int numPoints, unsigned int seed = 0);

    float noise(float x, float y) const;
};

class FractalNoise {
  private:
    PerlinNoise base;
    int octaves;
    float persistence;

  public:
    FractalNoise(int o, float p);

    float noise(float x, float y) const;
};

class Noise {
  public:
    static float perlin(float x, float y);
    static float simplex(float x, float y);
    static float worley(float x, float y);
    static float fractal(float x, float y, int octaves, float persistence);

    static float seed;
    static bool initializedSeed;
    static bool useSeed;
};

class Terrain;
class TerrainGenerator {
  public:
    virtual ~TerrainGenerator() = default;

    virtual float generateHeight(float x, float y) = 0;

    virtual void applyTo(Terrain &terrain) const {};
};

class HillGenerator : public TerrainGenerator {
  private:
    float scale;
    float amplitude;

  public:
    HillGenerator(float scale = 0.01f, float amplitude = 10.0f)
        : scale(scale), amplitude(amplitude) {}

    float generateHeight(float x, float y) override {
        float noise = Noise::perlin(x / scale, y / scale);
        return (noise + 1.0f) * 0.5f * amplitude / 10.0f;
    }
};

class MountainGenerator : public TerrainGenerator {
  private:
    float scale;
    float amplitude;
    int octaves;
    float persistence;

  public:
    MountainGenerator(float scale = 10.f, float amplitude = 100.0f,
                      int octaves = 5, float persistence = 0.5f)
        : scale(scale), amplitude(amplitude), octaves(octaves),
          persistence(persistence) {}

    float generateHeight(float x, float y) override {
        float noise =
            Noise::fractal(x * scale, y * scale, octaves, persistence);
        float height = noise;
        return height * amplitude;
    }
};

class PlainGenerator : public TerrainGenerator {
  private:
    float amplitude;
    float scale;

  public:
    PlainGenerator(float scale = 0.02f, float amplitude = 2.0f)
        : scale(scale), amplitude(amplitude) {}

    float generateHeight(float x, float y) override {
        float noise = Noise::perlin(x * scale, y * scale);
        return (noise + 1.0f) * 0.5f * amplitude / 2.0f;
    }
};

class IslandGenerator : public TerrainGenerator {
  private:
    int numFeatures;
    float scale;
    float amplitude;

  public:
    IslandGenerator(int numFeatures = 10, float scale = 0.01f,
                    float amplitude = 30.0f)
        : numFeatures(numFeatures), scale(scale), amplitude(amplitude) {}

    float generateHeight(float x, float y) override {
        WorleyNoise worley(numFeatures);
        float noise = worley.noise(x * scale, y * scale);
        return std::clamp(noise, 0.0f, 1.0f);
    }
};

class CompoundGenerator : public TerrainGenerator {
  private:
    std::vector<std::shared_ptr<TerrainGenerator>> generators;

  public:
    template <typename T>
        requires std::is_base_of<TerrainGenerator, T>::value
    void addGenerator(T gen) {
        generators.push_back(std::make_shared<T>(std::move(gen)));
    }

    float generateHeight(float x, float y) override {
        float height = 0.0f;
        for (auto &g : generators) {
            height += g->generateHeight(x, y);
        }
        return height;
    }
};

#endif // AURORA_PROCEDURAL_H