#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 lightViewProj;
    float maxPeak;
    float seaLevel;
    uint isFromMap;
};

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float3 FragPos [[user(locn1)]];
    float Height [[user(locn2)]];
    float4 FragPosLightSpace [[user(locn3)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float2 TextureCoord [[attribute(0)]];
    float4 gl_Position [[attribute(1)]];
};

struct main0_patchIn
{
    patch_control_point<main0_in> gl_in;
};

[[ patch(quad, 0) ]] vertex main0_out main0(main0_patchIn patchIn [[stage_in]], constant Uniforms& _84 [[buffer(0)]], constant UBO& _145 [[buffer(1)]], texture2d<float> heightMap [[texture(0)]], sampler heightMapSmplr [[sampler(0)]], float2 gl_TessCoordIn [[position_in_patch]])
{
    main0_out out = {};
    float3 gl_TessCoord = float3(gl_TessCoordIn.x, gl_TessCoordIn.y, 0.0);
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float2 t00 = patchIn.gl_in[0].TextureCoord;
    float2 t01 = patchIn.gl_in[1].TextureCoord;
    float2 t10 = patchIn.gl_in[2].TextureCoord;
    float2 t11 = patchIn.gl_in[3].TextureCoord;
    float2 t0 = ((t01 - t00) * u) + t00;
    float2 t1 = ((t11 - t10) * u) + t10;
    float2 texCoord = ((t1 - t0) * v) + t0;
    out.Height = (heightMap.sample(heightMapSmplr, texCoord, level(0.0)).x * _84.maxPeak) - _84.seaLevel;
    float4 p00 = patchIn.gl_in[0].gl_Position;
    float4 p01 = patchIn.gl_in[1].gl_Position;
    float4 p10 = patchIn.gl_in[2].gl_Position;
    float4 p11 = patchIn.gl_in[3].gl_Position;
    float4 p0 = ((p01 - p00) * u) + p00;
    float4 p1 = ((p11 - p10) * u) + p10;
    float4 position = ((p1 - p0) * v) + p0;
    position.y += out.Height;
    out.gl_Position = ((_145.projection * _145.view) * _145.model) * position;
    out.TexCoord = texCoord;
    out.FragPos = float3((_145.model * position).xyz);
    out.FragPosLightSpace = (_84.lightViewProj * _145.model) * position;
    return out;
}

