/*
 convex.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions for convex shapes
 Copyright (c) 2025 maxvdec
*/

#include "bezel/bounds.h"
#include "bezel/shape.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>

Bounds Convex::getBounds(const glm::vec3 &pos,
                         const glm::quat &orientation) const {
    glm::vec3 corners[8];
    corners[0] = glm::vec3(bounds.mins.x, bounds.mins.y, bounds.mins.z);
    corners[1] = glm::vec3(bounds.mins.x, bounds.mins.y, bounds.maxs.z);
    corners[2] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.mins.z);
    corners[3] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.maxs.z);
    corners[4] = glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.maxs.z);
    corners[5] = glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.mins.z);
    corners[6] = glm::vec3(bounds.maxs.x, bounds.mins.y, bounds.maxs.z);
    corners[7] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.maxs.z);

    Bounds bounds;
    for (int i = 0; i < 8; i++) {
        corners[i] = orientation * corners[i] + pos;
        bounds.expand(corners[i]);
    }

    return bounds;
}

float Convex::fastestLinearSpeed(const glm::vec3 &angularVelocity,
                                 const glm::vec3 &dir) const {
    float maxSpeed = 0.0f;
    for (const auto &v : vertices) {
        glm::vec3 r = v - centerOfMass;
        glm::vec3 linearVel = glm::cross(angularVelocity, r);
        float speed = glm::dot(linearVel, dir);
        if (speed > maxSpeed) {
            maxSpeed = speed;
        }
    }

    return maxSpeed;
}

int bezel::findFurthestPointInDirection(const std::vector<glm::vec3> &points,
                                        const glm::vec3 &dir) {
    int maxIdx = 0;
    float maxDist = glm::dot(points[0], dir);
    for (int i = 1; i < points.size(); i++) {
        const glm::vec3 &v = points[i];
        float dist = glm::dot(v, dir);
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return maxIdx;
}

float bezel::distanceFromLine(const glm::vec3 &a, const glm::vec3 &b,
                              const glm::vec3 &pt) {
    glm::vec3 ab = b - a;
    ab = glm::normalize(ab);

    glm::vec3 ray = pt - a;
    glm::vec3 proj = glm::dot(ray, ab) * ab;
    glm::vec3 perp = ray - proj;
    return glm::length(perp);
}

glm::vec3 bezel::findFurthestPointFromLine(const std::vector<glm::vec3> &points,
                                           const glm::vec3 &a,
                                           const glm::vec3 &b) {
    int maxIdx = 0;
    float maxDist = distanceFromLine(a, b, points[0]);
    for (int i = 1; i < points.size(); i++) {
        float dist = distanceFromLine(a, b, points[i]);
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return points[maxIdx];
}

float bezel::distanceFromTriangle(const glm::vec3 &a, const glm::vec3 &b,
                                  const glm::vec3 &c, const glm::vec3 &pt) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 normal = glm::cross(ab, ac);
    normal = glm::normalize(normal);

    glm::vec3 ray = pt - a;
    float dist = glm::dot(ray, normal);
    return dist;
}

glm::vec3
bezel::findFurthestPointFromTriangle(const std::vector<glm::vec3> &points,
                                     const glm::vec3 &a, const glm::vec3 &b,
                                     const glm::vec3 &c) {
    int maxIdx = 0;
    float maxDist = std::abs(distanceFromTriangle(a, b, c, points[0]));
    for (int i = 1; i < points.size(); i++) {
        float dist = std::abs(distanceFromTriangle(a, b, c, points[i]));
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return points[maxIdx];
}

void bezel::computeTetrahedron(std::vector<glm::vec3> vert,
                               std::vector<glm::vec3> &hullPts,
                               std::vector<Triangle> &hullTris) {
    hullPts.clear();
    hullTris.clear();

    glm::vec3 points[4];

    int idx = findFurthestPointInDirection(vert, glm::vec3(1, 0, 0));
    points[0] = vert[idx];
    idx = findFurthestPointInDirection(vert, points[0] * -1.0f);
    points[1] = vert[idx];
    points[2] = findFurthestPointFromLine(vert, points[0], points[1]);
    points[3] =
        findFurthestPointFromTriangle(vert, points[0], points[1], points[2]);

    float dist =
        distanceFromTriangle(points[0], points[1], points[2], points[3]);
    if (dist > 0.0f) {
        std::swap(points[0], points[1]);
    }

    hullPts.push_back(points[0]);
    hullPts.push_back(points[1]);
    hullPts.push_back(points[2]);
    hullPts.push_back(points[3]);

    Triangle t1 = {0, 1, 2};
    Triangle t2 = {0, 2, 3};
    Triangle t3 = {2, 1, 3};
    Triangle t4 = {1, 0, 3};

    hullTris.push_back(t1);
    hullTris.push_back(t2);
    hullTris.push_back(t3);
    hullTris.push_back(t4);
}
