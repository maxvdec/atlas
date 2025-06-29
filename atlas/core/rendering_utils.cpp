/*
 rendering_utils.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Rendering utilities
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/rendering.hpp"
#include "atlas/units.hpp"

CoreObject generateCubeObject(Position3d position, Size3d size) {
    std::vector<CoreVertex> vertices = {
        // Front face
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 0, 0)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(0, 1, 0)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(0, 0, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 0)},

        // Back face
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 0, 0)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(0, 1, 0)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(0, 0, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 0)},

        // Left face
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 0, 0)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(0, 1, 0)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(0, 0, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 0)},

        // Right
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 0, 0)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(0, 1, 0)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(0, 0, 1)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 1, 0)},

        // Top face
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(1, 0, 0)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z + size.depth / 2, Color(0, 1, 0)},
        {position.x + size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(0, 0, 1)},
        {position.x - size.width / 2, position.y + size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 0)},

        // Bottom face
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(1, 0, 0)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z + size.depth / 2, Color(0, 1, 0)},
        {position.x + size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(0, 0, 1)},
        {position.x - size.width / 2, position.y - size.height / 2,
         position.z - size.depth / 2, Color(1, 1, 0)},
    };

    CoreObject object(std::move(vertices));
    std::vector<Size2d> faceUVs = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    std::vector<Size2d> allUVs;
    for (int i = 0; i < 6; ++i) {
        allUVs.insert(allUVs.end(), faceUVs.begin(), faceUVs.end());
    }
    object.provideTextureCoords(allUVs);

    return object;
}
