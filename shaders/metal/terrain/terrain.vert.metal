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
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = ((_19.projection * _19.view) * _19.model) * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    return out;
}

