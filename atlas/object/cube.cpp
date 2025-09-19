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
        // Front face
        {{-w, -h, d}, color, {0.0, 0.0}},
        {{w, -h, d}, color, {1.0, 0.0}},
        {{w, h, d}, color, {1.0, 1.0}},
        {{-w, h, d}, color, {0.0, 1.0}},

        // Back face
        {{-w, -h, -d}, color, {1.0, 0.0}},
        {{-w, h, -d}, color, {1.0, 1.0}},
        {{w, h, -d}, color, {0.0, 1.0}},
        {{w, -h, -d}, color, {0.0, 0.0}},

        // Left face
        {{-w, -h, -d}, color, {0.0, 0.0}},
        {{-w, -h, d}, color, {1.0, 0.0}},
        {{-w, h, d}, color, {1.0, 1.0}},
        {{-w, h, -d}, color, {0.0, 1.0}},

        // Right face
        {{w, -h, -d}, color, {1.0, 0.0}},
        {{w, h, -d}, color, {1.0, 1.0}},
        {{w, h, d}, color, {0.0, 1.0}},
        {{w, -h, d}, color, {0.0, 0.0}},

        // Top face
        {{-w, h, -d}, color, {0.0, 1.0}},
        {{-w, h, d}, color, {0.0, 0.0}},
        {{w, h, d}, color, {1.0, 0.0}},
        {{w, h, -d}, color, {1.0, 1.0}},
        // Bottom face
        {{-w, -h, -d}, color, {1.0, 1.0}},
        {{w, -h, -d}, color, {0.0, 1.0}},
        {{w, -h, d}, color, {0.0, 0.0}},
        {{-w, -h, d}, color, {1.0, 0.0}},
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
