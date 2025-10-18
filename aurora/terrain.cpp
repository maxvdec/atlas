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

    glGenTextures(1, &terrainTexture.id);
    glBindTexture(GL_TEXTURE_2D, terrainTexture.id);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 nChannels == 4 ? GL_RGBA : (nChannels == 3 ? GL_RGB : GL_RED),
                 width, height, 0,
                 nChannels == 4 ? GL_RGBA : (nChannels == 3 ? GL_RGB : GL_RED),
                 GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    rez = 20;

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

    stbi_image_free(data);

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
    glBindVertexArray(this->vao);
    glUseProgram(terrainShader.programId);

    terrainShader.setUniformMat4f("model", model);
    terrainShader.setUniformMat4f("view", view);
    terrainShader.setUniformMat4f("projection", projection);

    terrainShader.setUniform1i("heightMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainTexture.id);

    glDrawArrays(GL_PATCHES, 0, patch_count * rez * rez);
    glBindVertexArray(0);
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