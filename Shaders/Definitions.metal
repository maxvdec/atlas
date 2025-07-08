//
//  Definitions.metal
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

#include <metal_stdlib>
#include <metal_matrix>
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float2 texCoords [[attribute(2)]];
    float3 normals [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 fragPosition;
    float4 color;
    float2 texCoords;
    float3 normals;
};

struct BasicUniforms {
    int textureCount;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct PhongUniforms {
    int textureCount;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 ambientColor;
    int lightCount;
};

#define POINT_LIGHT 0

struct Light {
    uint type;
    float3 color;
    float3 position;
    float intensity;
};
