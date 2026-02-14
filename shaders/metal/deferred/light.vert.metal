#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

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

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.TexCoord = in.aTexCoord;
    out.gl_Position = float4(in.aPos, 1.0);
    return out;
}

