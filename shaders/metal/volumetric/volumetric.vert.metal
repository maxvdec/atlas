#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 TexCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoords [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.aPos, 1.0);
    out.TexCoords = in.aTexCoords;
    return out;
}

