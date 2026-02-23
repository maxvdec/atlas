#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 projection;
};

struct main0_out
{
    float2 texCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 vertex0 [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant Uniforms& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = _19.projection * float4(in.vertex0.xy, 0.0, 1.0);
    out.texCoords = in.vertex0.zw;
    return out;
}

