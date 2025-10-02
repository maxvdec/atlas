/*
 shape.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shape creation functions
 Copyright (c) 2025 Max Van den Eynde
*/

#include "bezel/shape.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "bezel/body.h"
#include <memory>
#include <vector>

CoreObject createBox(Size3d size, Color color) {
    double w = size.x / 2.0;
    double h = size.y / 2.0;
    double d = size.z / 2.0;

    std::vector<CoreVertex> vertices = {
        // Front face (normal 0,0,1) - looking at +Z
        {{-w, -h, d}, color, {0.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, -h, d}, color, {1.0, 0.0}, {0.0f, 0.0f, 1.0f}},
        {{w, h, d}, color, {1.0, 1.0}, {0.0f, 0.0f, 1.0f}},
        {{-w, h, d}, color, {0.0, 1.0}, {0.0f, 0.0f, 1.0f}},

        // Back face (normal 0,0,-1) - looking at -Z
        {{w, -h, -d}, color, {0.0, 0.0}, {0.0f, 0.0f, -1.0f}},
        {{-w, -h, -d}, color, {1.0, 0.0}, {0.0f, 0.0f, -1.0f}},
        {{-w, h, -d}, color, {1.0, 1.0}, {0.0f, 0.0f, -1.0f}},
        {{w, h, -d}, color, {0.0, 1.0}, {0.0f, 0.0f, -1.0f}},

        // Left face (normal -1,0,0) - looking at -X
        {{-w, -h, -d}, color, {0.0, 0.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, -h, d}, color, {1.0, 0.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, h, d}, color, {1.0, 1.0}, {-1.0f, 0.0f, 0.0f}},
        {{-w, h, -d}, color, {0.0, 1.0}, {-1.0f, 0.0f, 0.0f}},

        // Right face (normal 1,0,0) - looking at +X
        {{w, -h, d}, color, {0.0, 0.0}, {1.0f, 0.0f, 0.0f}},
        {{w, -h, -d}, color, {1.0, 0.0}, {1.0f, 0.0f, 0.0f}},
        {{w, h, -d}, color, {1.0, 1.0}, {1.0f, 0.0f, 0.0f}},
        {{w, h, d}, color, {0.0, 1.0}, {1.0f, 0.0f, 0.0f}},

        // Top face (normal 0,1,0) - looking at +Y
        {{-w, h, d}, color, {0.0, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{w, h, d}, color, {1.0, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{w, h, -d}, color, {1.0, 1.0}, {0.0f, 1.0f, 0.0f}},
        {{-w, h, -d}, color, {0.0, 1.0}, {0.0f, 1.0f, 0.0f}},

        // Bottom face (normal 0,-1,0) - looking at -Y
        {{-w, -h, -d}, color, {0.0, 0.0}, {0.0f, -1.0f, 0.0f}},
        {{w, -h, -d}, color, {1.0, 0.0}, {0.0f, -1.0f, 0.0f}},
        {{w, -h, d}, color, {1.0, 1.0}, {0.0f, -1.0f, 0.0f}},
        {{-w, -h, d}, color, {0.0, 1.0}, {0.0f, -1.0f, 0.0f}},
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
    double w = size.width / 2.0;
    double h = size.height / 2.0;

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

CoreObject createPyramid(Size3d size, Color color) {
    double w = size.x / 2.0;
    double h = size.y;
    double d = size.z / 2.0;

    glm::vec3 apexVec{0.0, h, 0.0};
    Position3d apex = Position3d::fromGlm(apexVec);

    glm::vec3 blVec{-w, 0.0, -d};
    glm::vec3 brVec{w, 0.0, -d};
    glm::vec3 trVec{w, 0.0, d};
    glm::vec3 tlVec{-w, 0.0, d};

    Position3d bl = Position3d::fromGlm(blVec);
    Position3d br = Position3d::fromGlm(brVec);
    Position3d tr = Position3d::fromGlm(trVec);
    Position3d tl = Position3d::fromGlm(tlVec);

    std::vector<CoreVertex> vertices = {
        // Base (normal down) - reversed winding for CCW when viewed from below
        {bl, color, {0.0, 0.0}, Normal3d{0.0, -1.0, 0.0}},
        {tl, color, {0.0, 1.0}, Normal3d{0.0, -1.0, 0.0}},
        {tr, color, {1.0, 1.0}, Normal3d{0.0, -1.0, 0.0}},
        {br, color, {1.0, 0.0}, Normal3d{0.0, -1.0, 0.0}},

        // Side 1 (bl, br, apex) - front face
        {bl,
         color,
         {0.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(brVec - blVec, apexVec - blVec)))},
        {br,
         color,
         {1.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(brVec - blVec, apexVec - blVec)))},
        {apex,
         color,
         {0.5, 1.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(brVec - blVec, apexVec - blVec)))},

        // Side 2 (br, tr, apex) - right face
        {br,
         color,
         {0.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(trVec - brVec, apexVec - brVec)))},
        {tr,
         color,
         {1.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(trVec - brVec, apexVec - brVec)))},
        {apex,
         color,
         {0.5, 1.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(trVec - brVec, apexVec - brVec)))},

        // Side 3 (tr, tl, apex) - back face
        {tr,
         color,
         {0.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(tlVec - trVec, apexVec - trVec)))},
        {tl,
         color,
         {1.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(tlVec - trVec, apexVec - trVec)))},
        {apex,
         color,
         {0.5, 1.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(tlVec - trVec, apexVec - trVec)))},

        // Side 4 (tl, bl, apex) - left face
        {tl,
         color,
         {0.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(blVec - tlVec, apexVec - tlVec)))},
        {bl,
         color,
         {1.0, 0.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(blVec - tlVec, apexVec - tlVec)))},
        {apex,
         color,
         {0.5, 1.0},
         Normal3d::fromGlm(
             glm::normalize(glm::cross(blVec - tlVec, apexVec - tlVec)))}};

    std::vector<unsigned int> indices = {0, 1, 2, 2,  3,  0,  4,  5,  6,
                                         7, 8, 9, 10, 11, 12, 13, 14, 15};

    CoreObject pyramid;
    pyramid.attachVertices(vertices);
    pyramid.attachIndices(indices);

    return pyramid;
}

CoreObject createSphere(double radius, unsigned int sectorCount,
                        unsigned int stackCount, Color color) {
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;

    const double PI = 3.141592653589793;

    for (unsigned int i = 0; i <= stackCount; ++i) {
        double stackAngle = PI / 2 - i * (PI / stackCount);
        double xy = radius * cos(stackAngle);
        double z = radius * sin(stackAngle);

        for (unsigned int j = 0; j <= sectorCount; ++j) {
            double sectorAngle = j * (2 * PI / sectorCount);

            double x = xy * cos(sectorAngle);
            double y = xy * sin(sectorAngle);

            glm::vec3 pos((float)x, (float)y, (float)z);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 uv((float)j / sectorCount, (float)i / stackCount);

            CoreVertex v;
            v.position = Position3d(pos.x, pos.y, pos.z);
            v.normal = Normal3d(normal.x, normal.y, normal.z);
            v.textureCoordinate = {uv.x, uv.y};
            v.color = color;

            vertices.push_back(v);
        }
    }

    for (unsigned int i = 0; i < stackCount; ++i) {
        unsigned int k1 = i * (sectorCount + 1);
        unsigned int k2 = k1 + sectorCount + 1;

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    CoreObject sphere;
    sphere.attachVertices(vertices);
    sphere.attachIndices(indices);
    sphere.initialize();
    return sphere;
}

CoreObject createDebugPlane(Size2d size) {
    CoreObject plane = createPlane(size, Color::white());

    Color whiteMultiplier = Color(1.0, 1.0, 1.0);
    Color mediumMultiplier = Color(0.75, 0.75, 0.75);
    Color darkMultiplier = Color(0.5, 0.5, 0.5);

    Color blue = Color(0.5, 0.5, 1.0);

    Texture checkerboard = Texture::createDoubleCheckerboard(
        1024, 1024, 160, 20, blue * whiteMultiplier, blue * darkMultiplier,
        blue * mediumMultiplier);

    plane.attachTexture(checkerboard);
    return plane;
}

CoreObject createDebugSphere(double radius, unsigned int sectorCount,
                             unsigned int stackCount) {
    CoreObject sphere = createSphere(radius, sectorCount, stackCount);

    Color whiteMultiplier = Color(1.0, 1.0, 1.0);
    Color mediumMultiplier = Color(0.75, 0.75, 0.75);
    Color darkMultiplier = Color(0.5, 0.5, 0.5);

    Color blue = Color(0.5, 0.5, 1.0);
    Color red = Color(1.0, 0.5, 0.5);
    Color green = Color(0.5, 1.0, 0.5);

    std::vector<CheckerTile> tiles;
    tiles.push_back(
        CheckerTile{blue * whiteMultiplier, blue * whiteMultiplier, 80});
    tiles.push_back(
        CheckerTile{red * whiteMultiplier, red * whiteMultiplier, 80});
    tiles.push_back(
        CheckerTile{green * whiteMultiplier, green * whiteMultiplier, 80});

    Texture checkerboard = Texture::createTiledCheckerboard(1024, 1024, tiles);

    sphere.attachTexture(checkerboard);
    sphere.material.specular = Color::white() * 0.5;

    Body body;
    std::shared_ptr<Sphere> physicsSphere = std::make_shared<Sphere>(radius);
    body.shape = physicsSphere;
    body.invMass = 1.0f;

    sphere.setupPhysics(body);

    return sphere;
}

CoreObject createDebugBox(Size3d size) {
    CoreObject box = createBox(size, Color::white());

    Color whiteMultiplier = Color(1.0, 1.0, 1.0);
    Color mediumMultiplier = Color(0.75, 0.75, 0.75);
    Color darkMultiplier = Color(0.5, 0.5, 0.5);

    Color blue = Color(0.5, 0.5, 1.0);
    Color red = Color(1.0, 0.5, 0.5);
    Color green = Color(0.5, 1.0, 0.5);

    std::vector<CheckerTile> tiles;
    tiles.push_back(
        CheckerTile{blue * whiteMultiplier, blue * whiteMultiplier, 80});
    tiles.push_back(
        CheckerTile{red * whiteMultiplier, red * whiteMultiplier, 80});
    tiles.push_back(
        CheckerTile{green * whiteMultiplier, green * whiteMultiplier, 80});

    Texture checkerboard = Texture::createTiledCheckerboard(1024, 1024, tiles);

    box.attachTexture(checkerboard);
    box.material.specular = Color::white() * 0.5;

    std::vector<glm::vec3> corners = {{-size.x / 2, -size.y / 2, -size.z / 2},
                                      {size.x / 2, -size.y / 2, -size.z / 2},
                                      {size.x / 2, size.y / 2, -size.z / 2},
                                      {-size.x / 2, size.y / 2, -size.z / 2},
                                      {-size.x / 2, -size.y / 2, size.z / 2},
                                      {size.x / 2, -size.y / 2, size.z / 2},
                                      {size.x / 2, size.y / 2, size.z / 2},
                                      {-size.x / 2, size.y / 2, size.z / 2}};

    Body body;
    std::shared_ptr<Box> physicsBox = std::make_shared<Box>(corners);
    body.shape = physicsBox;
    body.invMass = 1.0f;

    box.setupPhysics(body);

    return box;
}