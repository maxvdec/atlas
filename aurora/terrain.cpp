//
// terrain.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Terrain functions implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <glad/glad.h>
#include "atlas/core/shader.h"
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
    vertexShader.compile();
    fragmentShader.compile();
    terrainShader = ShaderProgram(vertexShader, fragmentShader);
    terrainShader.compile();

    if (heightmap.type != ResourceType::Image) {
        throw std::runtime_error("Heightmap resource is not an image");
    }

    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *data = stbi_load(heightmap.path.string().c_str(), &width,
                                    &height, &nChannels, 0);
    if (data == nullptr) {
        std::cerr << "Failed to load heightmap!" << std::endl;
        return;
    }

    int detailMult = std::max(1, detail);
    int smoothStep = std::max(1, smoothness);

    int effectiveWidth = ((width - 1) * detailMult) / smoothStep + 1;
    int effectiveHeight = ((height - 1) * detailMult) / smoothStep + 1;

    std::cout << "Heightmap dimensions: " << width << "x" << height
              << std::endl;
    std::cout << "Detail level: " << detailMult << "x" << std::endl;
    std::cout << "Smoothness level: " << smoothStep << std::endl;
    std::cout << "Effective dimensions: " << effectiveWidth << "x"
              << effectiveHeight << std::endl;

    vertices.reserve(effectiveWidth * effectiveHeight * 3);
    float yScale = 64.0f / 256.0f;
    float yShift = 16.0f;

    auto getHeight = [&](float fi, float fj) -> float {
        int i0 = static_cast<int>(fi);
        int j0 = static_cast<int>(fj);
        int i1 = std::min(i0 + 1, height - 1);
        int j1 = std::min(j0 + 1, width - 1);

        float ti = fi - i0;
        float tj = fj - j0;

        unsigned char h00 = data[(i0 * width + j0) * nChannels];
        unsigned char h10 = data[(i1 * width + j0) * nChannels];
        unsigned char h01 = data[(i0 * width + j1) * nChannels];
        unsigned char h11 = data[(i1 * width + j1) * nChannels];

        float h0 = h00 * (1.0f - ti) + h10 * ti;
        float h1 = h01 * (1.0f - ti) + h11 * ti;
        float h = h0 * (1.0f - tj) + h1 * tj;

        return h;
    };

    for (int i = 0; i < effectiveHeight; i++) {
        for (int j = 0; j < effectiveWidth; j++) {
            float heightmapI =
                (i * smoothStep) / static_cast<float>(detailMult);
            float heightmapJ =
                (j * smoothStep) / static_cast<float>(detailMult);

            heightmapI = std::min(heightmapI, static_cast<float>(height - 1));
            heightmapJ = std::min(heightmapJ, static_cast<float>(width - 1));

            float heightValue = getHeight(heightmapI, heightmapJ);

            vertices.push_back(-height / 2.0f + heightmapI);
            vertices.push_back(-(heightValue * yScale - yShift));
            vertices.push_back(-width / 2.0f + heightmapJ);
        }
    }
    stbi_image_free(data);

    indices.reserve((effectiveHeight - 1) * effectiveWidth * 2);
    for (int i = 0; i < effectiveHeight - 1; i++) {
        for (int j = 0; j < effectiveWidth; j++) {
            indices.push_back((i + 1) * effectiveWidth + j);
            indices.push_back(i * effectiveWidth + j);
        }
    }

    strip_count = effectiveHeight - 1;
    vertices_per_strip = effectiveWidth * 2;

    std::cout << "Total vertices: " << vertices.size() / 3 << std::endl;
    std::cout << "Total indices: " << indices.size() << std::endl;

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &this->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Terrain::render(float dt) {
    glBindVertexArray(this->vao);
    glUseProgram(terrainShader.programId);

    terrainShader.setUniformMat4f("model", model);
    terrainShader.setUniformMat4f("view", view);
    terrainShader.setUniformMat4f("projection", projection);

    for (int strip = 0; strip < strip_count; ++strip) {
        glDrawElements(
            GL_TRIANGLE_STRIP, vertices_per_strip, GL_UNSIGNED_INT,
            (void *)(sizeof(unsigned int) * strip * vertices_per_strip));
    }
}