#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Paramters {
    float4x4 projection;
    float4x4 inverseProjection;
    float4x4 view;
    float2 noiseScale;
};

struct Samples {
    float3 samples[64];
};

struct main0_out {
    float FragColor [[color(0)]];
};

struct main0_in {
    float2 TexCoord [[user(locn0)]];
};

static inline float3 reconstructViewPos(float2 uv, float depth,
                                        float4x4 inverseProjection) {
    float2 ndc;
    ndc.x = uv.x * 2.0 - 1.0;
    ndc.y = (1.0 - uv.y) * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    float4 clip = float4(ndc, ndcZ, 1.0);
    float4 viewPos = inverseProjection * clip;
    float invW = 1.0 / max(abs(viewPos.w), 1e-6);
    return viewPos.xyz * invW;
}

fragment main0_out main0(main0_in in [[stage_in]],
                         constant Paramters &_43 [[buffer(0)]],
                         constant Samples &_137 [[buffer(1)]],
                         texture2d<float> gPosition [[texture(0)]],
                         texture2d<float> gNormal [[texture(1)]],
                         texture2d<float> texNoise [[texture(2)]],
                         sampler gPositionSmplr [[sampler(0)]],
                         sampler gNormalSmplr [[sampler(1)]],
                         sampler texNoiseSmplr [[sampler(2)]]) {
    main0_out out = {};
    float3 sampledNormalWorld = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    float sampledNormalLength = length(sampledNormalWorld);
    if (!all(isfinite(sampledNormalWorld)) ||
        sampledNormalLength <= 0.001000000047497451305389404296875) {
        out.FragColor = 1.0;
        return out;
    }
    float4 gPositionCenter = gPosition.sample(gPositionSmplr, in.TexCoord);
    float centerDepth = gPositionCenter.w;
    if (!isfinite(centerDepth)) {
        out.FragColor = 1.0;
        return out;
    }
    centerDepth = clamp(centerDepth, 0.0, 1.0);
    float3 fragPos = reconstructViewPos(in.TexCoord, centerDepth,
                                        _43.inverseProjection);
    float3 normalWorld = sampledNormalWorld / sampledNormalLength;
    float3 normal = fast::normalize((_43.view * float4(normalWorld, 0.0)).xyz);
    float3 randomVec = fast::normalize(
        (texNoise.sample(texNoiseSmplr, (in.TexCoord * _43.noiseScale)).xyz *
         2.0) -
        float3(1.0));
    float3 tangent =
        fast::normalize(randomVec - (normal * dot(randomVec, normal)));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(float3(tangent), float3(bitangent), float3(normal));
    float viewDepth = abs(fragPos.z);
    float baseRadius =
        mix(0.550000011920928955078125, 2.2000000476837158203125,
            clamp(viewDepth / 140.0, 0.0, 1.0));
    float3 dPosDx = dfdx(fragPos);
    float3 dPosDy = dfdy(fragPos);
    float pixelWorldScale = max(length(dPosDx), length(dPosDy));
    if (!isfinite(pixelWorldScale)) {
        pixelWorldScale = 0.0;
    }
    float sampleRadius =
        clamp(max(baseRadius, pixelWorldScale * 6.0), 0.3499999940395355,
              12.0);
    float depthBias = clamp(
        max(mix(0.006000000052154064178466796875,
                0.0240000002086162567138671875,
                clamp(viewDepth / 180.0, 0.0, 1.0)),
            sampleRadius * 0.009999999776482582),
        0.003000000026077032, 0.07999999821186066);
    float occlusion = 0.0;
    int validSamples = 0;
    for (int i = 0; i < 64; i++) {
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
        if (!_185) {
            _192 = offset.x > 1.0;
        } else {
            _192 = _185;
        }
        bool _199;
        if (!_192) {
            _199 = offset.y < 0.0;
        } else {
            _199 = _192;
        }
        bool _206;
        if (!_199) {
            _206 = offset.y > 1.0;
        } else {
            _206 = _199;
        }
        if (_206) {
            continue;
        }
        float sampleDepth = gPosition.sample(gPositionSmplr, offset.xy).w;
        if (!isfinite(sampleDepth)) {
            continue;
        }
        sampleDepth = clamp(sampleDepth, 0.0, 1.0);
        float3 sampleViewPos =
            reconstructViewPos(offset.xy, sampleDepth, _43.inverseProjection);
        float rangeCheck =
            smoothstep(0.0, 1.0,
                       sampleRadius / (abs(fragPos.z - sampleViewPos.z) +
                                        0.0005000000237487256526947021484375));
        occlusion +=
            (float(sampleViewPos.z >= (samplePos.z + depthBias)) * rangeCheck);
        validSamples++;
    }
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }
    out.FragColor = occlusion;
    return out;
}
