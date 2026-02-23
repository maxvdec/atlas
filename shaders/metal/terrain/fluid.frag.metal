#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4 waterColor;
    packed_float3 cameraPos;
    float time;
    float4x4 projection;
    float4x4 view;
    float4x4 invProjection;
    float4x4 invView;
    float3 lightDirection;
    float3 lightColor;
};

struct Parameters
{
    float refractionStrength;
    float reflectionStrength;
    float depthFade;
    int hasNormalTexture;
    int hasMovementTexture;
    float3 windForce;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float3 WorldPos [[user(locn1)]];
    float3 WorldNormal [[user(locn2)]];
    float3 WorldTangent [[user(locn3)]];
    float3 WorldBitangent [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _19 [[buffer(0)]], constant Parameters& _68 [[buffer(1)]], texture2d<float> movementTexture [[texture(0)]], texture2d<float> normalTexture [[texture(1)]], texture2d<float> reflectionTexture [[texture(2)]], texture2d<float> refractionTexture [[texture(3)]], texture2d<float> sceneTexture [[texture(4)]], sampler movementTextureSmplr [[sampler(0)]], sampler normalTextureSmplr [[sampler(1)]], sampler reflectionTextureSmplr [[sampler(2)]], sampler refractionTextureSmplr [[sampler(3)]], sampler sceneTextureSmplr [[sampler(4)]])
{
    main0_out out = {};
    float3 normal = fast::normalize(in.WorldNormal);
    float3 viewDir = fast::normalize(float3(_19.cameraPos) - in.WorldPos);
    float4 clipSpace = (_19.projection * _19.view) * float4(in.WorldPos, 1.0);
    float3 ndc = clipSpace.xyz / float3(clipSpace.w);
    float2 screenUV = (ndc.xy * 0.5) + float2(0.5);
    float windStrength = length(_68.windForce);
    float2 _78;
    if (windStrength > 0.001000000047497451305389404296875)
    {
        _78 = fast::normalize(_68.windForce.xy);
    }
    else
    {
        _78 = float2(1.0, 0.0);
    }
    float2 windDir = _78;
    float waveSpeed = 0.1500000059604644775390625 + (windStrength * 0.300000011920928955078125);
    float waveAmplitude = 0.00999999977648258209228515625 + (windStrength * 0.0199999995529651641845703125);
    float waveFrequency = 30.0 + (windStrength * 10.0);
    float2 waveOffset;
    waveOffset.x = sin(((in.TexCoord.x * windDir.x) + (_19.time * waveSpeed)) * waveFrequency);
    waveOffset.y = cos(((in.TexCoord.y * windDir.y) - (_19.time * waveSpeed)) * waveFrequency);
    waveOffset *= waveAmplitude;
    float2 flowOffset = float2(0.0);
    if (_68.hasMovementTexture == 1)
    {
        float2 windUV = (_68.windForce.xy * _19.time) * 0.0500000007450580596923828125;
        float2 movementUV = (in.TexCoord * 2.0) + windUV;
        float2 movementSample = movementTexture.sample(movementTextureSmplr, movementUV).xy;
        movementSample = (movementSample * 2.0) - float2(1.0);
        flowOffset = (movementSample * windStrength) * 0.1500000059604644775390625;
        waveOffset += (flowOffset * 0.5);
    }
    if (_68.hasNormalTexture == 1)
    {
        float3 T = fast::normalize(in.WorldTangent);
        float3 B = fast::normalize(in.WorldBitangent);
        float3 N = fast::normalize(in.WorldNormal);
        float3x3 TBN = float3x3(float3(T), float3(B), float3(N));
        float normalSpeed = 0.02999999932944774627685546875 + (windStrength * 0.0500000007450580596923828125);
        float2 normalUV1 = ((in.TexCoord * 5.0) + (waveOffset * 10.0)) + ((_68.windForce.xy * _19.time) * normalSpeed);
        float2 normalUV2 = ((in.TexCoord * 3.0) - (waveOffset * 8.0)) - (((_68.windForce.xy * _19.time) * normalSpeed) * 0.800000011920928955078125);
        float3 normalMap1 = normalTexture.sample(normalTextureSmplr, normalUV1).xyz;
        float3 normalMap2 = normalTexture.sample(normalTextureSmplr, normalUV2).xyz;
        normalMap1 = (normalMap1 * 2.0) - float3(1.0);
        normalMap2 = (normalMap2 * 2.0) - float3(1.0);
        float3 blendedNormal = fast::normalize(normalMap1 + normalMap2);
        float3 worldSpaceNormal = fast::normalize(TBN * blendedNormal);
        float normalStrength = 0.5 + (windStrength * 0.300000011920928955078125);
        normal = fast::normalize(mix(N, worldSpaceNormal, float3(normalStrength)));
    }
    float2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;
    reflectionUV += (waveOffset * 0.300000011920928955078125);
    float2 refractionUV = screenUV - (waveOffset * 0.20000000298023223876953125);
    bool _324 = reflectionUV.x >= 0.0;
    bool _330;
    if (_324)
    {
        _330 = reflectionUV.x <= 1.0;
    }
    else
    {
        _330 = _324;
    }
    bool _336;
    if (_330)
    {
        _336 = reflectionUV.y >= 0.0;
    }
    else
    {
        _336 = _330;
    }
    bool _342;
    if (_336)
    {
        _342 = reflectionUV.y <= 1.0;
    }
    else
    {
        _342 = _336;
    }
    bool reflectionInBounds = _342;
    bool _346 = refractionUV.x >= 0.0;
    bool _352;
    if (_346)
    {
        _352 = refractionUV.x <= 1.0;
    }
    else
    {
        _352 = _346;
    }
    bool _358;
    if (_352)
    {
        _358 = refractionUV.y >= 0.0;
    }
    else
    {
        _358 = _352;
    }
    bool _364;
    if (_358)
    {
        _364 = refractionUV.y <= 1.0;
    }
    else
    {
        _364 = _358;
    }
    bool refractionInBounds = _364;
    reflectionUV = fast::clamp(reflectionUV, float2(0.0500000007450580596923828125), float2(0.949999988079071044921875));
    refractionUV = fast::clamp(refractionUV, float2(0.0500000007450580596923828125), float2(0.949999988079071044921875));
    float2 reflectionEdgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), reflectionUV) * smoothstep(float2(1.0), float2(0.85000002384185791015625), reflectionUV);
    float reflectionFadeFactor = reflectionEdgeFade.x * reflectionEdgeFade.y;
    float2 refractionEdgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), refractionUV) * smoothstep(float2(1.0), float2(0.85000002384185791015625), refractionUV);
    float refractionFadeFactor = refractionEdgeFade.x * refractionEdgeFade.y;
    if (!reflectionInBounds)
    {
        reflectionFadeFactor *= 0.300000011920928955078125;
    }
    if (!refractionInBounds)
    {
        refractionFadeFactor *= 0.300000011920928955078125;
    }
    float4 reflectionSample = reflectionTexture.sample(reflectionTextureSmplr, reflectionUV);
    float4 refractionSample = refractionTexture.sample(refractionTextureSmplr, refractionUV);
    float4 sceneFallback = sceneTexture.sample(sceneTextureSmplr, screenUV);
    bool validReflection = length(reflectionSample.xyz) > 0.00999999977648258209228515625;
    bool validRefraction = length(refractionSample.xyz) > 0.00999999977648258209228515625;
    if (!validReflection)
    {
        reflectionSample = sceneFallback;
    }
    if (!validRefraction)
    {
        refractionSample = sceneFallback;
    }
    float fresnel = powr(1.0 - fast::clamp(dot(normal, viewDir), 0.0, 1.0), 3.0);
    fresnel = mix(0.0199999995529651641845703125, 1.0, fresnel);
    fresnel *= _68.reflectionStrength;
    float3 combined = mix(refractionSample.xyz, reflectionSample.xyz, float3(fresnel));
    float waterTint = fast::clamp(_68.depthFade * 0.300000011920928955078125, 0.0, 0.5);
    combined = mix(combined, _19.waterColor.xyz, float3(waterTint));
    float foamAmount = 0.0;
    if (_68.hasMovementTexture == 1)
    {
        float foamFactor = smoothstep(0.699999988079071044921875, 1.0, length(flowOffset) * windStrength);
        foamAmount = foamFactor * 0.20000000298023223876953125;
        combined = mix(combined, float3(1.0), float3(foamAmount));
    }
    float3 lightDir = fast::normalize(-_19.lightDirection);
    float diffuse = fast::max(dot(normal, lightDir), 0.0);
    diffuse *= 0.300000011920928955078125;
    float3 halfDir = fast::normalize(lightDir + viewDir);
    float specAngle = fast::max(dot(normal, halfDir), 0.0);
    float specular = powr(specAngle, 128.0);
    specular *= ((fresnel * 0.5) + 0.5);
    float waveVariation = (sin((in.TexCoord.x * 50.0) + (_19.time * 2.0)) * 0.5) + 0.5;
    waveVariation *= ((cos((in.TexCoord.y * 50.0) - (_19.time * 1.5)) * 0.5) + 0.5);
    specular *= (0.699999988079071044921875 + (waveVariation * 0.300000011920928955078125));
    float3 lighting = _19.lightColor * (diffuse + (specular * 2.0));
    combined += lighting;
    float3 specularHighlight = (_19.lightColor * specular) * 2.5;
    specularHighlight += float3(foamAmount * 2.0);
    float alpha = mix(0.699999988079071044921875, 0.949999988079071044921875, fresnel);
    out.FragColor = float4(combined, alpha);
    float luminance = fast::max(fast::max(combined.x, combined.y), combined.z);
    float3 bloomColor = float3(0.0);
    if (luminance > 1.0)
    {
        bloomColor = combined - float3(1.0);
    }
    bloomColor += specularHighlight;
    out.BrightColor = float4(bloomColor, alpha);
    return out;
}

