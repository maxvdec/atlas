#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Sun
{
    float2 sunPos;
};

struct VolumetricParameters
{
    float density;
    float weight;
    float decay;
    float exposure;
};

struct DirectionalLight
{
    float3 color;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoords [[user(locn0)]];
};

static inline __attribute__((always_inline))
float3 computeVolumetricLighting(thread const float2& uv, constant Sun& _21, constant VolumetricParameters& _31, texture2d<float> sceneTexture, sampler sceneTextureSmplr, constant DirectionalLight& directionalLight)
{
    float3 color = float3(0.0);
    float2 deltaTexCoord = (_21.sunPos - uv) * _31.density;
    float2 coord = uv;
    float illuminationDecay = 1.0;
    for (int i = 0; i < 100; i++)
    {
        coord += deltaTexCoord;
        bool _59 = coord.x < 0.0;
        bool _66;
        if (!_59)
        {
            _66 = coord.x > 1.0;
        }
        else
        {
            _66 = _59;
        }
        bool _74;
        if (!_66)
        {
            _74 = coord.y < 0.0;
        }
        else
        {
            _74 = _66;
        }
        bool _81;
        if (!_74)
        {
            _81 = coord.y > 1.0;
        }
        else
        {
            _81 = _74;
        }
        if (_81)
        {
            break;
        }
        float3 sampled = sceneTexture.sample(sceneTextureSmplr, coord).xyz;
        float brightness = dot(sampled, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float3 atmosphere = directionalLight.color * 0.0199999995529651641845703125;
        if (brightness > 0.5)
        {
            atmosphere += (sampled * 0.5);
        }
        atmosphere *= (illuminationDecay * _31.weight);
        color += atmosphere;
        illuminationDecay *= _31.decay;
    }
    return color * 5.0;
}

fragment main0_out main0(main0_in in [[stage_in]], constant Sun& _21 [[buffer(0)]], constant VolumetricParameters& _31 [[buffer(1)]], constant DirectionalLight& directionalLight [[buffer(2)]], texture2d<float> sceneTexture [[texture(0)]], sampler sceneTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 param = in.TexCoords;
    float3 rays = computeVolumetricLighting(param, _21, _31, sceneTexture, sceneTextureSmplr, directionalLight);
    out.FragColor = float4(rays, 1.0);
    return out;
}

