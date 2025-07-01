/*
 rendering_utils.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Rendering utilities
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/core/shaders.h"
#include "atlas/units.hpp"

CoreObject generateCubeObject(Position3d position, Size3d size) {
    std::vector<CoreVertex> vertices = {
        // Front face (Z+)
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},

        // Back face (Z-)
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},

        // Left face (X-)
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},

        // Right face (X+)
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},

        // Top face (Y+)
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},

        // Bottom face (Y-)
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 1)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 1)},
    };
    CoreObject object(std::move(vertices));
    std::vector<Size2d> faceUVs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    std::vector<Size2d> allUVs;
    for (int i = 0; i < 6; ++i) {
        allUVs.insert(allUVs.end(), faceUVs.begin(), faceUVs.end());
    }

    std::vector<uint32_t> indices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Left
        12, 13, 14, 14, 15, 12, // Right
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20  // Bottom
    };
    object.provideIndexedDrawing(std::move(indices));
    object.provideTextureCoords(allUVs);

    return object;
}

CoreObject presentFullScreenTexture(Texture texture) {
    CoreObject object;
    object.vertices = {
        {1.0f, 1.0f, 0.0f, Color(1, 1, 1), Size2d(1.0f, 1.0f)},
        {-1.0f, 1.0f, 0.0f, Color(1, 1, 1), Size2d(0.0f, 1.0f)},
        {-1.0f, -1.0f, 0.0f, Color(1, 1, 1), Size2d(0.0f, 0.0f)},

        {1.0f, 1.0f, 0.0f, Color(1, 1, 1), Size2d(1.0f, 1.0f)},
        {-1.0f, -1.0f, 0.0f, Color(1, 1, 1), Size2d(0.0f, 0.0f)},
        {1.0f, -1.0f, 0.0f, Color(1, 1, 1), Size2d(1.0f, 0.0f)},
    };
    object.addTexture(texture);
    object.fragmentShader =
        CoreShader(FULLSCREEN_FRAG, CoreShaderType::Fragment);
    object.vertexShader = CoreShader(FULLSCREEN_VERT, CoreShaderType::Vertex);
    return object;
}
