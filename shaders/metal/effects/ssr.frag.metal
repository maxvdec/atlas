#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 inverseProjection;
    float4x4 inverseView;
    float4x4 projection;
    float4x4 view;
    float3 cameraPosition;
};

struct SSRParameters
{
    float maxDistance;
    float resolution;
    int steps;
    float thickness;
    float maxRoughness;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline))
float3 fresnelSchlick(thread const float& cosTheta, thread const float3& F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline))
float4 SSR(thread const float3& worldPos, thread const float3& normal, thread const float3& viewDir, thread const float& roughness, thread const float& metallic, thread const float3& albedo, constant Uniforms& _42, constant SSRParameters& _96, thread float4& gl_FragCoord, texture2d<float> gPosition, sampler gPositionSmplr, texture2d<float> sceneColor, sampler sceneColorSmplr)
{
    float3 viewPos = (_42.view * float4(worldPos, 1.0)).xyz;
    float3 viewNormal = fast::normalize((_42.view * float4(normal, 0.0)).xyz);
    float3 viewDirection = fast::normalize(viewPos);
    float3 viewReflect = fast::normalize(reflect(viewDirection, viewNormal));
    if (viewReflect.z > 0.0)
    {
        return float4(0.0);
    }
    float3 rayOrigin = viewPos + (viewNormal * 0.00999999977648258209228515625);
    float3 rayDir = viewReflect;
    float stepSize = _96.maxDistance / float(_96.steps);
    float3 currentPos = rayOrigin;
    float3 magic = float3(0.067110560834407806396484375, 0.005837149918079376220703125, 52.98291778564453125);
    float jitter = fract(magic.z * fract(dot(gl_FragCoord.xy, magic.xy)));
    currentPos += ((rayDir * stepSize) * jitter);
    float2 hitUV = float2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    float3 lastPos = currentPos;
    for (int i = 0; i < _96.steps; i++)
    {
        float t = float(i) / float(_96.steps);
        float adaptiveStep = mix(0.300000011920928955078125, 1.0, t);
        currentPos += ((rayDir * stepSize) * adaptiveStep);
        float4 projectedPos = _42.projection * float4(currentPos, 1.0);
        float _186 = projectedPos.w;
        float4 _187 = projectedPos;
        float3 _190 = _187.xyz / float3(_186);
        projectedPos.x = _190.x;
        projectedPos.y = _190.y;
        projectedPos.z = _190.z;
        float2 screenUV = (projectedPos.xy * 0.5) + float2(0.5);
        bool _208 = screenUV.x < 0.0;
        bool _215;
        if (!_208)
        {
            _215 = screenUV.x > 1.0;
        }
        else
        {
            _215 = _208;
        }
        bool _222;
        if (!_215)
        {
            _222 = screenUV.y < 0.0;
        }
        else
        {
            _222 = _215;
        }
        bool _229;
        if (!_222)
        {
            _229 = screenUV.y > 1.0;
        }
        else
        {
            _229 = _222;
        }
        if (_229)
        {
            break;
        }
        float3 sampleWorldPos = gPosition.sample(gPositionSmplr, screenUV).xyz;
        float3 sampleViewPos = (_42.view * float4(sampleWorldPos, 1.0)).xyz;
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        float depthDiff = sampleDepth - currentDepth;
        bool _265 = depthDiff > 0.0;
        bool _272;
        if (_265)
        {
            _272 = depthDiff < _96.thickness;
        }
        else
        {
            _272 = _265;
        }
        if (_272)
        {
            float3 binarySearchStart = lastPos;
            float3 binarySearchEnd = currentPos;
            for (int j = 0; j < 8; j++)
            {
                float3 midPoint = (binarySearchStart + binarySearchEnd) * 0.5;
                float4 midProj = _42.projection * float4(midPoint, 1.0);
                float _303 = midProj.w;
                float4 _304 = midProj;
                float3 _307 = _304.xyz / float3(_303);
                midProj.x = _307.x;
                midProj.y = _307.y;
                midProj.z = _307.z;
                float2 midUV = (midProj.xy * 0.5) + float2(0.5);
                float3 midSampleWorldPos = gPosition.sample(gPositionSmplr, midUV).xyz;
                float3 midSampleViewPos = (_42.view * float4(midSampleWorldPos, 1.0)).xyz;
                float midSampleDepth = -midSampleViewPos.z;
                float midCurrentDepth = -midPoint.z;
                if (midCurrentDepth < midSampleDepth)
                {
                    binarySearchStart = midPoint;
                }
                else
                {
                    binarySearchEnd = midPoint;
                }
            }
            float4 finalProj = _42.projection * float4(binarySearchEnd, 1.0);
            float _364 = finalProj.w;
            float4 _365 = finalProj;
            float3 _368 = _365.xyz / float3(_364);
            finalProj.x = _368.x;
            finalProj.y = _368.y;
            finalProj.z = _368.z;
            hitUV = (finalProj.xy * 0.5) + float2(0.5);
            hitDepth = length(currentPos - rayOrigin);
            hit = true;
            break;
        }
        lastPos = currentPos;
    }
    if (!hit)
    {
        return float4(0.0);
    }
    float mipLevel = roughness * 5.0;
    float3 reflectionColor = sceneColor.sample(sceneColorSmplr, hitUV, level(mipLevel)).xyz;
    float2 edgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), hitUV) * (float2(1.0) - smoothstep(float2(0.85000002384185791015625), float2(1.0), hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    float distanceFade = 1.0 - smoothstep(_96.maxDistance * 0.5, _96.maxDistance, hitDepth);
    float roughnessFade = 1.0 - smoothstep(0.0, _96.maxRoughness, roughness);
    float3 F0 = mix(float3(0.039999999105930328369140625), albedo, float3(metallic));
    float3 V = fast::normalize(_42.cameraPosition - worldPos);
    float cosTheta = fast::max(dot(normal, V), 0.0);
    float param = cosTheta;
    float3 param_1 = F0;
    float3 fresnel = fresnelSchlick(param, param_1);
    float fresnelFactor = ((fresnel.x + fresnel.y) + fresnel.z) / 3.0;
    float finalFade = ((edgeFactor * distanceFade) * roughnessFade) * fresnelFactor;
    return float4(reflectionColor, finalFade);
}

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _42 [[buffer(0)]], constant SSRParameters& _96 [[buffer(1)]], texture2d<float> gPosition [[texture(0)]], texture2d<float> sceneColor [[texture(1)]], texture2d<float> gNormal [[texture(2)]], texture2d<float> gAlbedoSpec [[texture(3)]], texture2d<float> gMaterial [[texture(4)]], sampler gPositionSmplr [[sampler(0)]], sampler sceneColorSmplr [[sampler(1)]], sampler gNormalSmplr [[sampler(2)]], sampler gAlbedoSpecSmplr [[sampler(3)]], sampler gMaterialSmplr [[sampler(4)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float3 worldPos = gPosition.sample(gPositionSmplr, in.TexCoord).xyz;
    float3 normal = fast::normalize(gNormal.sample(gNormalSmplr, in.TexCoord).xyz);
    float3 albedo = gAlbedoSpec.sample(gAlbedoSpecSmplr, in.TexCoord).xyz;
    float4 material = gMaterial.sample(gMaterialSmplr, in.TexCoord);
    float metallic = material.x;
    float roughness = material.y;
    if (length(normal) < 0.001000000047497451305389404296875)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    if (roughness > _96.maxRoughness)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    float3 viewDir = fast::normalize(_42.cameraPosition - worldPos);
    float3 param = worldPos;
    float3 param_1 = normal;
    float3 param_2 = viewDir;
    float param_3 = roughness;
    float param_4 = metallic;
    float3 param_5 = albedo;
    float4 reflection = SSR(param, param_1, param_2, param_3, param_4, param_5, _42, _96, gl_FragCoord, gPosition, gPositionSmplr, sceneColor, sceneColorSmplr);
    out.FragColor = reflection;
    return out;
}

