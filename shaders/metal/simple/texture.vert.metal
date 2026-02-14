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
    float4 outColor [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float2 aTexCoord [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _13 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 mvp = (_13.projection * _13.view) * _13.model;
    out.gl_Position = mvp * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    out.outColor = in.aColor;
    return out;
}

