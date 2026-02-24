//
// noise.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Noise types and implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "aurora/procedural.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <vector>

float PerlinNoise::fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

float PerlinNoise::lerp(float t, float a, float b) { return a + (t * (b - a)); }

float PerlinNoise::grad(int hash, float x, float y) {
    int h = hash & 3;
    float u = (h & 1) ? -x : x;
    float v = (h & 2) ? -y : y;
    return u + v;
}

std::vector<int> PerlinNoise::p;
std::vector<int> PerlinNoise::permutation;

PerlinNoise::PerlinNoise(unsigned int seed) {
    if (p.empty() && permutation.empty()) {
        p.resize(512);
        std::vector<int> perm(256);
        for (int i = 0; i < 256; i++) {
            perm[i] = i;
        }

        std::default_random_engine generator(seed);
        std::uniform_int_distribution<int> distribution(0, 255);
        for (int i = 255; i > 0; i--) {
            int j = distribution(generator) % (i + 1);
            std::swap(perm[i], perm[j]);
        }

        for (int i = 0; i < 512; i++) {
            p[i] = perm[i % 256];
        }
    }
}

float PerlinNoise::noise(float x, float y) const {
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    float u = fade(x);
    float v = fade(y);

    int A = p[X] + Y;
    int AA = p[A];
    int AB = p[A + 1];
    int B = p[X + 1] + Y;
    int BA = p[B];
    int BB = p[B + 1];

    float res =
        lerp(lerp(grad(p[AA], x, y), grad(p[BA], x - 1, y), u),
             lerp(grad(p[AB], x, y - 1), grad(p[BB], x - 1, y - 1), u), v);

    return res;
}

const int SimplexNoise::grad3[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

int SimplexNoise::fastfloor(float x) { return x > 0 ? (int)x : (int)x - 1; }

float SimplexNoise::dot(const int *g, float x, float y) {
    return (g[0] * x) + (g[1] * y);
}

float SimplexNoise::noise(float xin, float yin) {
    const float F2 = 0.5f * (sqrt(3.0f) - 1.0f);
    const float G2 = (3.0f - sqrt(3.0f)) / 6.0f;

    float s = (xin + yin) * F2;
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);

    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = xin - X0;
    float y0 = yin - Y0;

    int i1, j1;
    if (x0 > y0) {
        i1 = 1;
        j1 = 0;
    } else {
        i1 = 0;
        j1 = 1;
    }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + (2.0f * G2);
    float y2 = y0 - 1.0f + (2.0f * G2);

    int ii = i & 255;

    float t0 = 0.5f - (x0 * x0) - (y0 * y0);
    float n0 = t0 < 0 ? 0.0f : pow(t0, 4) * dot(grad3[ii % 12], x0, y0);

    float t1 = 0.5f - (x1 * x1) - (y1 * y1);
    float n1 = t1 < 0 ? 0.0f : pow(t1, 4) * dot(grad3[(ii + i1) % 12], x1, y1);

    float t2 = 0.5f - (x2 * x2) - (y2 * y2);
    float n2 = t2 < 0 ? 0.0f : pow(t2, 4) * dot(grad3[(ii + 1) % 12], x2, y2);

    return 70.0f * (n0 + n1 + n2);
}

WorleyNoise::WorleyNoise(int numPoints, unsigned int seed)
    : numPoints(numPoints) {
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    for (int i = 0; i < this->numPoints; i++) {
        float x = distribution(generator);
        float y = distribution(generator);
        featurePoints.emplace_back(x, y);
    }
}

float WorleyNoise::noise(float x, float y) const {
    float minDist = std::numeric_limits<float>::max();
    for (const auto &point : featurePoints) {
        float dx = x - point.first;
        float dy = y - point.second;
        float dist = sqrt((dx * dx) + (dy * dy));
        minDist = std::min(dist, minDist);
    }
    return minDist;
}

FractalNoise::FractalNoise(int o, float p)
    : base(0), octaves(o), persistence(p) {}

float FractalNoise::noise(float x, float y) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += base.noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    return total / maxValue;
}

bool Noise::useSeed = false;
bool Noise::initializedSeed = false;
float Noise::seed = 0.0f;

float Noise::perlin(float x, float y) {
    if (useSeed) {
        return PerlinNoise(static_cast<unsigned int>(seed)).noise(x, y);
    } else if (!initializedSeed) {
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<unsigned int> distribution(0, 10000);
        seed = static_cast<float>(distribution(generator));
        Noise::initializedSeed = true;
        return PerlinNoise(static_cast<unsigned int>(seed)).noise(x, y);
    } else {
        return PerlinNoise(static_cast<unsigned int>(seed)).noise(x, y);
    }
}

float Noise::simplex(float x, float y) { return SimplexNoise::noise(x, y); }

float Noise::worley(float x, float y) {
    if (useSeed) {
        return WorleyNoise(16, static_cast<unsigned int>(seed)).noise(x, y);
    } else if (!initializedSeed) {
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<unsigned int> distribution(0, 10000);
        seed = static_cast<float>(distribution(generator));
        return WorleyNoise(16, static_cast<unsigned int>(seed)).noise(x, y);
    } else {
        return WorleyNoise(16, static_cast<unsigned int>(seed)).noise(x, y);
    }
}

float Noise::fractal(float x, float y, int octaves, float persistence) {
    FractalNoise fractalNoise(octaves, persistence);
    return fractalNoise.noise(x, y);
}
