#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Paramters
{
    float4x4 projection;
    float4x4 view;
    float2 noiseScale;
};

struct Samples
{
    float3 samples[64];
};

struct main0_out
{
    float FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Paramters& _43 [[buffer(0)]], constant Samples& _137 [[buffer(1)]], texture2d<float> gPosition [[texture(0)]], texture2d<float> gNormal [[texture(1)]], texture2d<float> texNoise [[texture(2)]], sampler gPositionSmplr [[sampler(0)]], sampler gNormalSmplr [[sampler(1)]], sampler texNoiseSmplr [[sampler(2)]])
{
    main0_out out = {};
    float3 fragPosWorld = gPosition.sample(gPositionSmplr, in.TexCoord).xyz;
    if (!all(isfinite(fragPosWorld)))
    {
        out.FragColor = 1.0;
        return out;
    }
    float3 geometricNormal = cross(dfdx(fragPosWorld), dfdy(fragPosWorld));
    float geometricNormalLength = length(geometricNormal);
    bool hasGeometricNormal = all(isfinite(geometricNormal)) && geometricNormalLength > 0.001000000047497451305389404296875;
    float3 sampledNormalWorld = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    float sampledNormalLength = length(sampledNormalWorld);
    bool hasSampledNormal = all(isfinite(sampledNormalWorld)) && sampledNormalLength > 0.001000000047497451305389404296875;
    float3 normalWorld;
    if (hasGeometricNormal)
    {
        normalWorld = geometricNormal / geometricNormalLength;
        if (hasSampledNormal)
        {
            float3 shadingNormal = sampledNormalWorld / sampledNormalLength;
            if (dot(normalWorld, shadingNormal) < 0.0)
            {
                normalWorld = -normalWorld;
            }
        }
    }
    else
    {
        if (!hasSampledNormal)
        {
            out.FragColor = 1.0;
            return out;
        }
        normalWorld = sampledNormalWorld / sampledNormalLength;
    }
    float3 fragPos = (_43.view * float4(fragPosWorld, 1.0)).xyz;
    float3 normal = fast::normalize((_43.view * float4(normalWorld, 0.0)).xyz);
    float3 randomVec = fast::normalize((texNoise.sample(texNoiseSmplr, (in.TexCoord * _43.noiseScale)).xyz * 2.0) - float3(1.0));
    float3 tangent = fast::normalize(randomVec - (normal * dot(randomVec, normal)));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(float3(tangent), float3(bitangent), float3(normal));
    float viewDepth = abs(fragPos.z);
    float sampleRadius = mix(0.550000011920928955078125, 2.2000000476837158203125, clamp(viewDepth / 140.0, 0.0, 1.0));
    float depthBias = mix(0.006000000052154064178466796875, 0.0240000002086162567138671875, clamp(viewDepth / 180.0, 0.0, 1.0));
    float occlusion = 0.0;
    int validSamples = 0;
    for (int i = 0; i < 64; i++)
    {
        float3 samplePos = TBN * _137.samples[i];
        samplePos = fragPos + (samplePos * sampleRadius);
        float4 offset = _43.projection * float4(samplePos, 1.0);
        float _160 = offset.w;
        float4 _161 = offset;
        float3 _164 = _161.xyz / float3(_160);
        offset.x = _164.x;
        offset.y = _164.y;
        offset.z = _164.z;
        float4 _174 = offset;
        float2 _178 = (_174.xy * 0.5) + float2(0.5);
        offset.x = _178.x;
        offset.y = 1.0 - _178.y;
        bool _185 = offset.x < 0.0;
        bool _192;
        if (!_185)
        {
            _192 = offset.x > 1.0;
        }
        else
        {
            _192 = _185;
        }
        bool _199;
        if (!_192)
        {
            _199 = offset.y < 0.0;
        }
        else
        {
            _199 = _192;
        }
        bool _206;
        if (!_199)
        {
            _206 = offset.y > 1.0;
        }
        else
        {
            _206 = _199;
        }
        if (_206)
        {
            continue;
        }
        float3 samplePosWorld = gPosition.sample(gPositionSmplr, offset.xy).xyz;
        if (!all(isfinite(samplePosWorld)))
        {
            continue;
        }
        float sampleDepth = (_43.view * float4(samplePosWorld, 1.0)).z;
        float rangeCheck = smoothstep(0.0, 1.0, sampleRadius / (abs(fragPos.z - sampleDepth) + 0.0005000000237487256526947021484375));
        occlusion += (float(sampleDepth >= (samplePos.z + depthBias)) * rangeCheck);
        validSamples++;
    }
    if (validSamples > 0)
    {
        occlusion = 1.0 - (occlusion / float(validSamples));
    }
    else
    {
        occlusion = 1.0;
    }
    out.FragColor = occlusion;
    return out;
}
