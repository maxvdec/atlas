//
// gjk.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: GJK algorithm implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/body.h"
#include "bezel/shape.h"
#include <array>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

bool bezel::simplexSignedVolumes(const std::vector<Point> &simplex,
                                 glm::vec3 &newDir, glm::vec4 &lambdasOut) {
    const float epsilonf = 1e-6f;
    lambdasOut = glm::vec4(0.0f);

    bool doesIntersect = false;
    switch (simplex.size()) {
    default:
    case 2: {
        glm::vec2 lambdas = bezel::projectOn1D(simplex[0].xyz, simplex[1].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 2; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
    } break;
    case 3: {
        glm::vec3 lambdas =
            bezel::projectOn2D(simplex[0].xyz, simplex[1].xyz, simplex[2].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 3; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
        lambdasOut[2] = lambdas[2];
    } break;
    case 4: {
        glm::vec4 lambdas = bezel::projectOn3D(simplex[0].xyz, simplex[1].xyz,
                                               simplex[2].xyz, simplex[3].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 4; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
        lambdasOut[2] = lambdas[2];
        lambdasOut[3] = lambdas[3];
    } break;
    }

    return doesIntersect;
}

bool bezel::hasPoint(const std::array<Point, 4> &simplex, const Point &p) {
    const float precision = 1e-6f;

    for (const auto &sp : simplex) {
        glm::vec3 delta = sp.xyz - p.xyz;
        if (glm::length2(delta) < precision * precision) {
            return true;
        }
    }
    return false;
}

void bezel::sortValids(std::array<Point, 4> &simplex, glm::vec4 &lambdas) {
    bool valids[4];
    for (int i = 0; i < 4; i++) {
        valids[i] = true;
        if (lambdas[i] == 0.0f) {
            valids[i] = false;
        }
    }

    glm::vec4 validLambdas(0.0f);
    int validCount = 0;
    std::array<Point, 4> validPoints;
    for (int i = 0; i < 4; i++) {
        if (valids[i]) {
            validPoints[validCount] = simplex[i];
            validLambdas[validCount] = lambdas[i];
            validCount++;
        }
    }

    for (int i = 0; i < 4; i++) {
        simplex[i] = validPoints[i];
        lambdas[i] = validLambdas[i];
    }
}

static int bezel::numValids(const glm::vec4 &lambdas) {
    int num = 0;
    for (int i = 0; i < 4; i++) {
        if (lambdas[i] != 0.0f) {
            num++;
        }
    }
    return num;
}

bool bezel::gjkIntersection(const std::shared_ptr<Body> bodyA,
                            const std::shared_ptr<Body> bodyB) {
    const glm::vec3 origin(0.0);

    int numPts = 1;
    std::array<Point, 4> simplex;
    simplex[0] =
        bezel::support(bodyA, bodyB, glm::vec3(1.0f, 0.0f, 0.0f), 0.0f);

    float closestDist = 1e10f;
    bool doesContainOrigin = false;
    glm::vec3 newDir = -simplex[0].xyz;
    while (!doesContainOrigin) {
        Point newPt = bezel::support(bodyA, bodyB, newDir, 0.0f);

        if (hasPoint(simplex, newPt)) {
            break;
        }

        simplex[numPts] = newPt;
        numPts++;

        float dotdot = glm::dot(newDir, newPt.xyz - origin);
        if (dotdot < 0.0f) {
            break;
        }

        glm::vec4 lambdas;
        doesContainOrigin = bezel::simplexSignedVolumes(
            std::vector<Point>(simplex.begin(), simplex.begin() + numPts),
            newDir, lambdas);
        if (doesContainOrigin) {
            break;
        }

        float dist = glm::length2(newDir);
        if (dist >= closestDist) {
            break;
        }
        closestDist = dist;

        bezel::sortValids(simplex, lambdas);
        numPts = bezel::numValids(lambdas);
        doesContainOrigin = (numPts == 4);
    }

    return doesContainOrigin;
}