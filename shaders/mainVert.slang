//
// mainVert.slang
// As part of the shaders project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Main Vertex Shader
// Copyright (c) 2025 maxvdec

#include "lib/math.slang"

struct VertexInput {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float4x4 instanceModel : INSTANCE_TRANSFORM;
}

struct VertexOut {
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
    float3x3 TBN : TEXCOORD2;
}

uniform float4x4 model;
uniform float4x4 view;
uniform float4x4 projection;
uniform bool isInstanced = true;

[shader("vertex")]
func main(VertexInput input)->VertexOut {
    float4x4 modelMatrix = input.instanceModel;
    if (isInstanced) {
        modelMatrix = input.instanceModel;
    }

    float4x4 mvp = projection * view * modelMatrix;
    VertexOut output;

    output.position = mul(mvp, float4(input.position, 1.0));
    output.color = input.color;
    output.texCoord = input.texCoord;

    float3x3 normalMatrix = float3x3(transpose(inverse(modelMatrix)));
    output.normal = normalize(mul(normalMatrix, input.normal));
    float3 N = output.normal;
    float3 T = normalize(mul(normalMatrix, input.tangent));
    float3 B = normalize(mul(normalMatrix, input.bitangent));
    output.TBN = float3x3(T, B, N);

    return output;
}
