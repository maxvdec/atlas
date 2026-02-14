#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    packed_float3 lightPos;
    float far_plane;
};

struct main0_out
{
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float4 FragPos [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _17 [[buffer(0)]])
{
    main0_out out = {};
    float lightDistance = length(in.FragPos.xyz - float3(_17.lightPos));
    lightDistance /= _17.far_plane;
    out.gl_FragDepth = lightDistance;
    return out;
}

