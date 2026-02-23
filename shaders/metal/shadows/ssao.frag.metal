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
    float3 normalWorld = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    if (length(normalWorld) < 0.001000000047497451305389404296875)
    {
        out.FragColor = 1.0;
        return out;
    }
    float3 fragPos = (_43.view * float4(fragPosWorld, 1.0)).xyz;
    float3 normal = fast::normalize((_43.view * float4(normalWorld, 0.0)).xyz);
    float3 randomVec = fast::normalize((texNoise.sample(texNoiseSmplr, (in.TexCoord * _43.noiseScale)).xyz * 2.0) - float3(1.0));
    float3 tangent = fast::normalize(randomVec - (normal * dot(randomVec, normal)));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(float3(tangent), float3(bitangent), float3(normal));
    float occlusion = 0.0;
    int validSamples = 0;
    for (int i = 0; i < 64; i++)
    {
        float3 samplePos = TBN * _137.samples[i];
        samplePos = fragPos + (samplePos * 0.5);
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
        float sampleDepth = (_43.view * float4(samplePosWorld, 1.0)).z;
        float rangeCheck = smoothstep(0.0, 1.0, 0.5 / (abs(fragPos.z - sampleDepth) + 0.001000000047497451305389404296875));
        occlusion += (float(sampleDepth >= (samplePos.z + 0.02500000037252902984619140625)) * rangeCheck);
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

