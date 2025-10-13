//
// ssao.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: SSAO Implementation for the Window Class
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/window.h"
#include <glm/gtc/random.hpp>
#include <random>

void Window::setupSSAO() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) / 64.0f;
        scale = 0.1f + 0.9f * (scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0,
                        randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }

    unsigned int noiseTextureID;
    glGenTextures(1, &noiseTextureID);
    glBindTexture(GL_TEXTURE_2D, noiseTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT,
                 &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    noiseTexture.id = noiseTextureID;
    noiseTexture.creationData.width = 4;
    noiseTexture.creationData.height = 4;
    noiseTexture.type = TextureType::SSAONoise;
}