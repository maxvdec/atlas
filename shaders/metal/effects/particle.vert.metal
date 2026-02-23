#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UniformBufferObject
{
    float4x4 view;
    float4x4 projection;
    float4x4 model;
    uint isAmbient;
};

struct main0_out
{
    float2 fragTexCoord [[user(locn0)]];
    float4 fragColor [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 quadVertex [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float3 particlePos [[attribute(2)]];
    float4 particleColor [[attribute(3)]];
    float particleSize [[attribute(4)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UniformBufferObject& _12 [[buffer(0)]])
{
    main0_out out = {};
    if (_12.isAmbient != 0u)
    {
        float4 viewParticlePos = _12.view * float4(in.particlePos, 1.0);
        float3 viewPosition = viewParticlePos.xyz + float3(in.quadVertex.x * in.particleSize, in.quadVertex.y * in.particleSize, 0.0);
        out.gl_Position = (_12.projection * _12.model) * float4(viewPosition, 1.0);
    }
    else
    {
        float3 cameraRight = float3(_12.view[0].x, _12.view[1].x, _12.view[2].x);
        float3 cameraUp = float3(_12.view[0].y, _12.view[1].y, _12.view[2].y);
        float3 worldPosition = (in.particlePos + ((cameraRight * in.quadVertex.x) * in.particleSize)) + ((cameraUp * in.quadVertex.y) * in.particleSize);
        out.gl_Position = (_12.projection * _12.view) * float4(worldPosition, 1.0);
    }
    out.fragTexCoord = in.texCoord;
    out.fragColor = in.particleColor;
    return out;
}

