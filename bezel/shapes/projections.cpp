/*
 projections.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Projection functions for the GJK algorithm
 Copyright (c) 2025 maxvdec
*/

#include "bezel/shape.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>

glm::vec2 bezel::projectOn1D(const glm::vec3 &s1, const glm::vec3 &s2) {
    glm::vec3 ab = s2 - s1;
    glm::vec3 ap = -s1;
    glm::vec3 p0 = ab * glm::dot(ab, ap) / glm::length2(ab);

    int idx = 0;
    float mu_max = 0;
    for (int i = 0; i < 3; i++) {
        float mu = s2[i] - s1[i];
        if (mu * mu > mu_max * mu_max) {
            mu_max = mu;
            idx = i;
        }
    }

    const float a = s1[idx];
    const float b = s2[idx];
    const float p = p0[idx];

    const float C1 = p - a;
    const float C2 = b - p;

    if ((p > a && p < b) || (p > b && p < a)) {
        glm::vec2 lambdas;
        lambdas[0] = C2 / mu_max;
        lambdas[1] = C1 / mu_max;
        return lambdas;
    }

    if ((a <= b && p <= a) || (a >= b && p >= a)) {
        return glm::vec2(1.0f);
    }

    return glm::vec2(0.0f, 1.0f);
}

int bezel::compareSigns(float a, float b) {
    if (a < 0 && b < 0)
        return 1;
    if (a > 0 && b > 0)
        return 1;
    return 0;
}

glm::vec3 bezel::projectOn2D(const glm::vec3 &s1, const glm::vec3 &s2,
                             const glm::vec3 &s3) {
    glm::vec3 normal = glm::cross(s2 - s1, s3 - s1);
    glm::vec3 p0 = normal * glm::dot(s1, normal) / glm::length2(normal);

    int idx = 0;
    float area_max = 0;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        glm::vec2 a = glm::vec2(s1[j], s1[k]);
        glm::vec2 b = glm::vec2(s2[j], s2[k]);
        glm::vec2 c = glm::vec2(s3[j], s3[k]);
        glm::vec2 ab = b - a;
        glm::vec2 ac = c - a;

        float area = ab.x * ac.y - ab.y * ac.x;
        if (area * area > area_max * area_max) {
            idx = i;
            area_max = area;
        }
    }

    int x = (idx + 1) % 3;
    int y = (idx + 2) % 3;
    glm::vec2 s[3];
    s[0] = glm::vec2(s1[x], s1[y]);
    s[1] = glm::vec2(s2[x], s2[y]);
    s[2] = glm::vec2(s3[x], s3[y]);
    glm::vec2 p(p0[x], p0[y]);

    glm::vec3 areas;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        glm::vec2 a = p;
        glm::vec2 b = s[j];
        glm::vec2 c = s[k];
        glm::vec2 ab = b - a;
        glm::vec2 ac = c - a;

        areas[i] = ab.x * ac.y - ab.y * ac.x;
    }

    if (compareSigns(area_max, areas[0]) && compareSigns(area_max, areas[1]) &&
        compareSigns(area_max, areas[2])) {
        glm::vec3 lambdas;
        lambdas[0] = areas[0] / area_max;
        lambdas[1] = areas[1] / area_max;
        lambdas[2] = areas[2] / area_max;
        return lambdas;
    }

    float dist = 1e10f;
    glm::vec3 lambdas = glm::vec3(1.0, 0.0, 0.0);
    for (int i = 0; i < 3; i++) {
        int k = (i + 1) % 3;
        int l = (i + 2) % 3;

        glm::vec3 edgesPts[3];
        edgesPts[0] = s1;
        edgesPts[1] = s2;
        edgesPts[2] = s3;

        glm::vec2 lambdaEdge = projectOn1D(edgesPts[k], edgesPts[l]);
        glm::vec3 pt =
            edgesPts[k] * lambdaEdge[0] + edgesPts[l] * lambdaEdge[1];
        if (glm::length2(pt) < dist) {
            dist = glm::length2(pt);
            lambdas = glm::vec3(0.0f);
            lambdas[k] = lambdaEdge[0];
            lambdas[l] = lambdaEdge[1];
        }
    }

    return lambdas;
}

glm::vec4 bezel::projectOn3D(const glm::vec3 &s1, const glm::vec3 &s2,
                             const glm::vec3 &s3, const glm::vec3 &s4) {
    glm::mat4 M;
    M[0] = glm::vec4(s1.x, s2.x, s3.x, s4.x);
    M[1] = glm::vec4(s1.y, s2.y, s3.y, s4.y);
    M[2] = glm::vec4(s1.z, s2.z, s3.z, s4.z);
    M[3] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    glm::vec4 C4;
    C4[0] = bezel::takeCofactor(M, 3, 0);
    C4[1] = bezel::takeCofactor(M, 3, 1);
    C4[2] = bezel::takeCofactor(M, 3, 2);
    C4[3] = bezel::takeCofactor(M, 3, 3);

    const float detM = C4[0] + C4[1] + C4[2] + C4[3];

    if (compareSigns(detM, C4[0]) && compareSigns(detM, C4[1]) &&
        compareSigns(detM, C4[2]) && compareSigns(detM, C4[3])) {
        glm::vec4 lambdas = C4 / detM;
        return lambdas;
    }

    glm::vec4 lambdas;
    float dist = 1e10;
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        int k = (i + 2) % 4;

        glm::vec3 edgesPts[4];
        edgesPts[0] = s1;
        edgesPts[1] = s2;
        edgesPts[2] = s3;
        edgesPts[3] = s4;

        glm::vec3 lambdasFace =
            projectOn2D(edgesPts[i], edgesPts[j], edgesPts[k]);
        glm::vec3 pt = edgesPts[i] * lambdasFace[0] +
                       edgesPts[j] * lambdasFace[1] +
                       edgesPts[k] * lambdasFace[2];
        if (glm::length2(pt) < dist) {
            dist = glm::length2(pt);
            lambdas = glm::vec4(0.0f);
            lambdas[i] = lambdasFace[0];
            lambdas[j] = lambdasFace[1];
            lambdas[k] = lambdasFace[2];
        }
    }

    return lambdas;
}
