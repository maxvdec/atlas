#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UniformBufferObject
{
    float4x4 projection;
    float4x4 view;
};

struct main0_out
{
    float3 TexCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UniformBufferObject& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.TexCoords = in.aPos;
    float3x3 _32 = float3x3(_19.view[0].xyz, _19.view[1].xyz, _19.view[2].xyz);
    float4x4 viewNoTranslation = float4x4(float4(_32[0].x, _32[0].y, _32[0].z, 0.0), float4(_32[1].x, _32[1].y, _32[1].z, 0.0), float4(_32[2].x, _32[2].y, _32[2].z, 0.0), float4(0.0, 0.0, 0.0, 1.0));
    float4 pos = (_19.projection * viewNoTranslation) * float4(in.aPos, 1.0);
    out.gl_Position = pos.xyww;
    return out;
}

