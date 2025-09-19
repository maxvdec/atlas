/*
 cube.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Simple cube definition and helper function
 Copyright (c) 2025 maxvdec
*/

#include "atlas/object.h"
#include "atlas/units.h"

CoreObject createBox(Size3d size, Color color) {
    double w = size.x / 2.0;
    double h = size.y / 2.0;
    double d = size.z / 2.0;

    std::vector<CoreVertex> vertices = {
        // Front face (normal 0,0,1)
        {{-w, -h, d}, color, {0.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, -h, d}, color, {1.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, h, d}, color, {1.0, 1.0}, {0.0f, 0.0f, 1.0f}},
        {{-w, h, d}, color, {0.0, 1.0}, {0.0f, 0.0f, 1.0f}},

        // Back face (normal 0,0,-1)
        {{-w, -h, -d}, color, {1.0, 0.0}, {0.0f, 0.0f, -1.0f}},
        {{-w, h, -d}, color, {1.0, 1.0}, {0.0f, 0.0f, -1.0f}},
        {{w, h, -d}, color, {0.0, 1.0}, {0.0f, 0.0f, -1.0f}},
        {{w, -h, -d}, color, {0.0, 0.0}, {0.0f, 0.0f, -1.0f}},

        // Left face (normal -1,0,0)
        {{-w, -h, -d}, color, {0.0, 0.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, -h, d}, color, {1.0, 0.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, h, d}, color, {1.0, 1.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, h, -d}, color, {0.0, 1.0}, {-1.0f, 0.0f, 0.0f}},

        // Right face (normal 1,0,0)
        {{w, -h, -d}, color, {1.0, 0.0}, {1.0f, 0.0f, 0.0f}},
        {{w, h, -d}, color, {1.0, 1.0}, {1.0f, 0.0f, 0.0f}},
        {{w, h, d}, color, {0.0, 1.0}, {1.0f, 0.0f, 0.0f}},
        {{w, -h, d}, color, {0.0, 0.0}, {1.0f, 0.0f, 0.0f}},

        // Top face (normal 0,1,0)
        {{-w, h, -d}, color, {0.0, 1.0}, {0.0f, 1.0f, 0.0f}},
        {{-w, h, d}, color, {0.0, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{w, h, d}, color, {1.0, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{w, h, -d}, color, {1.0, 1.0}, {0.0f, 1.0f, 0.0f}},

        // Bottom face (normal 0,-1,0)
        {{-w, -h, -d}, color, {1.0, 1.0}, {0.0f, -1.0f, 0.0f}},
        {{w, -h, -d}, color, {0.0, 1.0}, {0.0f, -1.0f, 0.0f}},
        {{w, -h, d}, color, {0.0, 0.0}, {0.0f, -1.0f, 0.0f}},
        {{-w, -h, d}, color, {1.0, 0.0}, {0.0f, -1.0f, 0.0f}},
    };

    CoreObject box;
    box.attachVertices(vertices);
    box.attachIndices({0,  1,  2,  2,  3,  0,    // Front face
                       4,  5,  6,  6,  7,  4,    // Back face
                       8,  9,  10, 10, 11, 8,    // Left face
                       12, 13, 14, 14, 15, 12,   // Right face
                       16, 17, 18, 18, 19, 16,   // Top face
                       20, 21, 22, 22, 23, 20}); // Bottom face
    return box;
}

CoreObject createPlane(Size2d size, Color color) {
    double w = size.x / 2.0;
    double h = size.y / 2.0;

    std::vector<CoreVertex> vertices = {
        {{-w, -h, 0.0}, color, {0.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, -h, 0.0}, color, {1.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, h, 0.0}, color, {1.0, 1.0}, {0.0f, 0.0f, 1.0f}},
        {{-w, h, 0.0}, color, {0.0, 1.0}, {0.0f, 0.0f, 1.0f}},
    };

    CoreObject plane;
    plane.attachVertices(vertices);
    plane.attachIndices({0, 1, 2, 2, 3, 0});
    plane.rotate({-90.0, 0.0, 0.0});
    return plane;
}
