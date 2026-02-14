#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float3 WorldPos [[user(locn1)]];
    float3 WorldNormal [[user(locn2)]];
    float3 WorldTangent [[user(locn3)]];
    float3 WorldBitangent [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(1)]];
    float3 aNormal [[attribute(2)]];
    float3 aTangent [[attribute(3)]];
    float3 aBitangent [[attribute(4)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = ((_19.projection * _19.view) * _19.model) * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    out.WorldPos = float3((_19.model * float4(in.aPos, 1.0)).xyz);
    out.WorldNormal = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aNormal);
    out.WorldTangent = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aTangent);
    out.WorldBitangent = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aBitangent);
    return out;
}

