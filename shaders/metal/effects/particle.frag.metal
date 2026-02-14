#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    uint useTexture;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 fragTexCoord [[user(locn0)]];
    float4 fragColor [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _9 [[buffer(0)]], texture2d<float> particleTexture [[texture(0)]], sampler particleTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    if (_9.useTexture != 0u)
    {
        float4 texColor = particleTexture.sample(particleTextureSmplr, in.fragTexCoord);
        if (texColor.w < 0.00999999977648258209228515625)
        {
            discard_fragment();
        }
        out.FragColor = texColor * in.fragColor;
    }
    else
    {
        float2 center = float2(0.5);
        float dist = distance(in.fragTexCoord, center);
        float alpha = 1.0 - smoothstep(0.300000011920928955078125, 0.5, dist);
        out.FragColor = float4(in.fragColor.xyz, in.fragColor.w * alpha);
        if (out.FragColor.w < 0.00999999977648258209228515625)
        {
            discard_fragment();
        }
    }
    return out;
}

