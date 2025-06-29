/*
 light.cpp - FIXED VERSION
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting functions and solutions (FIXED)
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.hpp"
#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/units.hpp"
#include <glm/glm.hpp>

Light::Light(Position3d position, Color color)
    : position(position), color(color) {

    this->debugObject = CoreObject({// Front face
                                    {-0.5f, -0.5f, 0.5f, Color(1, 0, 0)},
                                    {0.5f, -0.5f, 0.5f, Color(0, 1, 0)},
                                    {0.5f, 0.5f, 0.5f, Color(0, 0, 1)},
                                    {-0.5f, 0.5f, 0.5f, Color(1, 1, 0)},

                                    // Back face
                                    {0.5f, -0.5f, -0.5f, Color(1, 0, 0)},
                                    {-0.5f, -0.5f, -0.5f, Color(0, 1, 0)},
                                    {-0.5f, 0.5f, -0.5f, Color(0, 0, 1)},
                                    {0.5f, 0.5f, -0.5f, Color(1, 1, 0)},

                                    // Left face
                                    {-0.5f, -0.5f, -0.5f, Color(1, 0, 0)},
                                    {-0.5f, -0.5f, 0.5f, Color(0, 1, 0)},
                                    {-0.5f, 0.5f, 0.5f, Color(0, 0, 1)},
                                    {-0.5f, 0.5f, -0.5f, Color(1, 1, 0)},

                                    // Right face
                                    {0.5f, -0.5f, 0.5f, Color(1, 0, 0)},
                                    {0.5f, -0.5f, -0.5f, Color(0, 1, 0)},
                                    {0.5f, 0.5f, -0.5f, Color(0, 0, 1)},
                                    {0.5f, 0.5f, 0.5f, Color(1, 1, 0)},

                                    // Top face
                                    {-0.5f, 0.5f, 0.5f, Color(1, 0, 0)},
                                    {0.5f, 0.5f, 0.5f, Color(0, 1, 0)},
                                    {0.5f, 0.5f, -0.5f, Color(0, 0, 1)},
                                    {-0.5f, 0.5f, -0.5f, Color(1, 1, 0)},

                                    // Bottom face
                                    {-0.5f, -0.5f, -0.5f, Color(1, 0, 0)},
                                    {0.5f, -0.5f, -0.5f, Color(0, 1, 0)},
                                    {0.5f, -0.5f, 0.5f, Color(0, 0, 1)},
                                    {-0.5f, -0.5f, 0.5f, Color(1, 1, 0)}});

    std::vector<Size2d> faceUVs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    std::vector<Size2d> allUVs;
    for (int i = 0; i < 6; ++i) {
        allUVs.insert(allUVs.end(), faceUVs.begin(), faceUVs.end());
    }

    debugObject.provideTextureCoords(allUVs);

    std::vector<uint32_t> indices;
    for (int i = 0; i < 6; ++i) {
        uint32_t start = i * 4;
        indices.push_back(start + 0);
        indices.push_back(start + 1);
        indices.push_back(start + 2);
        indices.push_back(start + 2);
        indices.push_back(start + 3);
        indices.push_back(start + 0);
    }
    debugObject.provideIndexedDrawing(indices);

    debugObject.initialize();
    debugObject.hide();
}

void Light::debugLight() { this->debugObject.show(); }
