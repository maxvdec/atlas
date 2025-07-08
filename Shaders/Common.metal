//
//  Common.metal
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

#include <metal_stdlib>
using namespace metal;

float3x3 inverse3x3(float3x3 m) {
    float a = m[0][0], b = m[0][1], c = m[0][2];
    float d = m[1][0], e = m[1][1], f = m[1][2];
    float g = m[2][0], h = m[2][1], i = m[2][2];

    float A =   (e * i - f * h);
    float B = - (d * i - f * g);
    float C =   (d * h - e * g);
    float D = - (b * i - c * h);
    float E =   (a * i - c * g);
    float F = - (a * h - b * g);
    float G =   (b * f - c * e);
    float H = - (a * f - c * d);
    float I =   (a * e - b * d);

    float det = a * A + b * B + c * C;
    if (abs(det) < 1e-6) return float3x3(0); // non-invertible, return zero matrix

    float invDet = 1.0 / det;

    return float3x3(
        A * invDet, D * invDet, G * invDet,
        B * invDet, E * invDet, H * invDet,
        C * invDet, F * invDet, I * invDet
    );
}

float3x3 toFloat3x3(float4x4 m) {
    return float3x3(
        m[0].xyz,
        m[1].xyz,
        m[2].xyz
    );
}

