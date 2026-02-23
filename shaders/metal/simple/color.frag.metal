#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float4 vertexColor [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    float3 color = in.vertexColor.xyz / (in.vertexColor.xyz + float3(1.0));
    out.FragColor = float4(color, in.vertexColor.w);
    if (length(color) > 1.0)
    {
        out.BrightColor = float4(color, in.vertexColor.w);
    }
    return out;
}

