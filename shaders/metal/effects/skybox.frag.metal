#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float3 sunDirection;
    float4 sunColor;
    float3 moonDirection;
    float4 moonColor;
    int hasDayNight;
};

struct AtmosphereParams
{
    float sunTintStrength;
    float moonTintStrength;
    float sunSizeMultiplier;
    float moonSizeMultiplier;
    float starDensity;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float3 TexCoords [[user(locn0)]];
};

static inline __attribute__((always_inline))
float hash13(thread float3& p)
{
    p = fract(p * 443.897003173828125);
    p += float3(dot(p, p.yzx + float3(19.1900005340576171875)));
    return fract((p.x + p.y) * p.z);
}

static inline __attribute__((always_inline))
float3 generateStars(thread const float3& dir, thread const float& density, thread const float& nightFactor)
{
    if ((density <= 0.0) || (nightFactor <= 0.0))
    {
        return float3(0.0);
    }
    float3 starSpace = dir * 50.0;
    float3 cell = floor(starSpace);
    float3 localPos = fract(starSpace);
    float3 param = cell;
    float _165 = hash13(param);
    float rand = _165;
    if (rand < (density * 0.300000011920928955078125))
    {
        float3 param_1 = cell + float3(12.340000152587890625, 56.779998779296875, 90.12000274658203125);
        float _181 = hash13(param_1);
        float randX = _181;
        float3 param_2 = cell + float3(23.450000762939453125, 67.8899993896484375, 1.230000019073486328125);
        float _190 = hash13(param_2);
        float randY = _190;
        float3 param_3 = cell + float3(34.560001373291015625, 78.90000152587890625, 12.340000152587890625);
        float _198 = hash13(param_3);
        float randZ = _198;
        float3 starPos = float3(randX, randY, randZ);
        float dist = length(localPos - starPos);
        float3 param_4 = cell + float3(45.6699981689453125, 89.01000213623046875, 23.450000762939453125);
        float _217 = hash13(param_4);
        float starSize = 0.0199999995529651641845703125 + (_217 * 0.02999999932944774627685546875);
        float3 param_5 = cell + float3(56.779998779296875, 90.12000274658203125, 34.560001373291015625);
        float _227 = hash13(param_5);
        float brightness = 0.5 + (_227 * 0.5);
        float star = smoothstep(starSize, 0.0, dist) * brightness;
        float3 param_6 = cell + float3(67.8899993896484375, 1.230000019073486328125, 45.6699981689453125);
        float _243 = hash13(param_6);
        float twinkle = 0.800000011920928955078125 + (0.20000000298023223876953125 * sin(_243 * 100.0));
        star *= (twinkle * nightFactor);
        float3 starColor = float3(1.0);
        if (rand > 0.89999997615814208984375)
        {
            starColor = float3(0.800000011920928955078125, 0.89999997615814208984375, 1.0);
        }
        else
        {
            if (rand > 0.800000011920928955078125)
            {
                starColor = float3(1.0, 0.89999997615814208984375, 0.800000011920928955078125);
            }
        }
        return starColor * star;
    }
    return float3(0.0);
}

static inline __attribute__((always_inline))
float hash21(thread float2& p)
{
    p = fract(p * float2(123.339996337890625, 456.209991455078125));
    p += float2(dot(p, p + float2(45.31999969482421875)));
    return fract(p.x * p.y);
}

static inline __attribute__((always_inline))
float valueNoise(thread const float2& p)
{
    float2 i = floor(p);
    float2 f = fract(p);
    f = (f * f) * (float2(3.0) - (f * 2.0));
    float2 param = i;
    float _80 = hash21(param);
    float a = _80;
    float2 param_1 = i + float2(1.0, 0.0);
    float _88 = hash21(param_1);
    float b = _88;
    float2 param_2 = i + float2(0.0, 1.0);
    float _94 = hash21(param_2);
    float c = _94;
    float2 param_3 = i + float2(1.0);
    float _100 = hash21(param_3);
    float d = _100;
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

static inline __attribute__((always_inline))
float3 generateMoonTexture(thread float2& uv, thread const float& distanceFromCenter, thread const float3& tintColor)
{
    float angle = 0.5;
    float ca = cos(angle);
    float sa = sin(angle);
    uv = float2((ca * uv.x) - (sa * uv.y), (sa * uv.x) + (ca * uv.y));
    float2 param = uv * 2.0;
    float largeFeatures = valueNoise(param);
    largeFeatures = smoothstep(0.300000011920928955078125, 0.699999988079071044921875, largeFeatures);
    float2 param_1 = uv * 8.0;
    float mediumCraters = valueNoise(param_1);
    float2 craterUV = uv * 6.0;
    float2 craterCell = floor(craterUV);
    float2 craterLocal = fract(craterUV);
    float craters = 1.0;
    for (int i = 0; i < 4; i++)
    {
        float2 neighbor = float2(float(i % 2), float(i / 2));
        float2 cellPoint = craterCell + neighbor;
        float2 param_2 = cellPoint;
        float _353 = hash21(param_2);
        float2 param_3 = cellPoint + float2(13.69999980926513671875, 27.299999237060546875);
        float _360 = hash21(param_3);
        float2 craterCenter = float2(_353, _360);
        float dist = length((craterLocal - neighbor) - craterCenter);
        float2 param_4 = cellPoint + float2(5.30000019073486328125, 9.69999980926513671875);
        float _378 = hash21(param_4);
        float craterSize = 0.1500000059604644775390625 + (0.25 * _378);
        if (dist < craterSize)
        {
            float crater = smoothstep(craterSize, craterSize * 0.300000011920928955078125, dist);
            craters = fast::min(craters, 1.0 - (crater * 0.699999988079071044921875));
        }
    }
    float surface = (largeFeatures * 0.5) + (mediumCraters * 0.5);
    surface *= craters;
    float intensity = mix(0.300000011920928955078125, 0.75, surface);
    float limb = 1.0 - smoothstep(0.60000002384185791015625, 1.0, distanceFromCenter);
    intensity *= (0.4000000059604644775390625 + (0.60000002384185791015625 * limb));
    intensity *= 1.2999999523162841796875;
    return fast::clamp(tintColor * intensity, float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _452 [[buffer(0)]], constant AtmosphereParams& _484 [[buffer(1)]], texturecube<float> skybox [[texture(0)]], sampler skyboxSmplr [[sampler(0)]])
{
    main0_out out = {};
    float3 dir = fast::normalize(in.TexCoords);
    float3 color = skybox.sample(skyboxSmplr, in.TexCoords).xyz;
    if (_452.hasDayNight == 1)
    {
        float3 normSunDir = fast::normalize(_452.sunDirection);
        float3 normMoonDir = fast::normalize(_452.moonDirection);
        float sunDot = dot(dir, normSunDir);
        float moonDot = dot(dir, normMoonDir);
        float nightFactor = smoothstep(0.1500000059604644775390625, -0.20000000298023223876953125, _452.sunDirection.y);
        if (_484.starDensity > 0.0)
        {
            float3 param = dir;
            float param_1 = _484.starDensity;
            float param_2 = nightFactor;
            color += generateStars(param, param_1, param_2);
        }
        float sunHorizonFade = smoothstep(-0.1500000059604644775390625, 0.0500000007450580596923828125, _452.sunDirection.y);
        if (_452.sunDirection.y > (-0.1500000059604644775390625))
        {
            float sizeAdjust = 1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.001000000047497451305389404296875);
            float sunSize = 0.999499976634979248046875 * sizeAdjust;
            float sunGlowSize = 0.99800002574920654296875 * (1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.0030000000260770320892333984375));
            float sunHaloSize = 0.9900000095367431640625 * (1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.014999999664723873138427734375));
            float sunDisk = smoothstep(sunSize - 0.00019999999494757503271102905273438, sunSize, sunDot);
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
            float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) * (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
            float horizonBoost = smoothstep(0.100000001490116119384765625, -0.0500000007450580596923828125, _452.sunDirection.y) * 2.0;
            sunHalo *= (0.300000011920928955078125 + horizonBoost);
            color += ((_452.sunColor.xyz * (((sunDisk * 5.0) + (sunGlow * 0.5)) + sunHalo)) * sunHorizonFade);
        }
        float moonHorizonFade = smoothstep(-0.1500000059604644775390625, 0.0500000007450580596923828125, _452.moonDirection.y);
        if (_452.moonDirection.y > (-0.1500000059604644775390625))
        {
            float sizeAdjust_1 = 1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.001000000047497451305389404296875);
            float moonSize = 0.999599993228912353515625 * sizeAdjust_1;
            float moonGlowSize = 0.99849998950958251953125 * (1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.0030000000260770320892333984375));
            float moonHaloSize = 0.99199998378753662109375 * (1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.014999999664723873138427734375));
            float moonDisk = smoothstep(moonSize - 0.00019999999494757503271102905273438, moonSize, moonDot);
            if (moonDisk > 0.00999999977648258209228515625)
            {
                float3 up = (abs(normMoonDir.y) < 0.999000012874603271484375) ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
                float3 right = fast::normalize(cross(up, normMoonDir));
                float3 actualUp = cross(normMoonDir, right);
                float3 relativeDir = dir - (normMoonDir * moonDot);
                float u = dot(relativeDir, right);
                float v = dot(relativeDir, actualUp);
                float distFromCenter = length(float2(u, v)) / sqrt(1.0 - (moonSize * moonSize));
                if (distFromCenter < 1.0)
                {
                    float2 moonUV = float2(u, v) * 200.0;
                    float2 param_3 = moonUV;
                    float param_4 = distFromCenter;
                    float3 param_5 = _452.moonColor.xyz;
                    float3 _703 = generateMoonTexture(param_3, param_4, param_5);
                    float3 moonTexture = _703;
                    color += (((moonTexture * moonDisk) * 1.5) * moonHorizonFade);
                }
                else
                {
                    color += (((_452.moonColor.xyz * moonDisk) * 1.5) * moonHorizonFade);
                }
            }
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * (1.0 - moonDisk);
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            float moonHorizonBoost = smoothstep(0.100000001490116119384765625, -0.0500000007450580596923828125, _452.moonDirection.y) * 2.0;
            moonHalo *= (0.20000000298023223876953125 + moonHorizonBoost);
            color += ((_452.moonColor.xyz * ((moonGlow * 0.300000011920928955078125) + moonHalo)) * moonHorizonFade);
        }
        bool _767 = _452.sunDirection.y > (-0.100000001490116119384765625);
        bool _773;
        if (_767)
        {
            _773 = _484.sunTintStrength > 0.0;
        }
        else
        {
            _773 = _767;
        }
        if (_773)
        {
            float sunSkyInfluence = smoothstep(0.699999988079071044921875, 0.949999988079071044921875, sunDot) * smoothstep(-0.100000001490116119384765625, 0.20000000298023223876953125, _452.sunDirection.y);
            color = mix(color, color * _452.sunColor.xyz, float3((sunSkyInfluence * 0.300000011920928955078125) * _484.sunTintStrength));
            float sunProximity = smoothstep(0.300000011920928955078125, 0.800000011920928955078125, sunDot) * smoothstep(-0.100000001490116119384765625, 0.300000011920928955078125, _452.sunDirection.y);
            color += (((_452.sunColor.xyz * sunProximity) * 0.1500000059604644775390625) * _484.sunTintStrength);
            float globalSunTint = smoothstep(-0.100000001490116119384765625, 0.5, _452.sunDirection.y);
            color = mix(color, _452.sunColor.xyz, float3((globalSunTint * _484.sunTintStrength) * 0.07999999821186065673828125));
        }
        bool _833 = _452.moonDirection.y > (-0.100000001490116119384765625);
        bool _839;
        if (_833)
        {
            _839 = _484.moonTintStrength > 0.0;
        }
        else
        {
            _839 = _833;
        }
        if (_839)
        {
            float moonSkyInfluence = smoothstep(0.800000011920928955078125, 0.949999988079071044921875, moonDot) * smoothstep(-0.100000001490116119384765625, 0.20000000298023223876953125, _452.moonDirection.y);
            color = mix(color, color * _452.moonColor.xyz, float3((moonSkyInfluence * 0.20000000298023223876953125) * _484.moonTintStrength));
            float moonProximity = smoothstep(0.5, 0.85000002384185791015625, moonDot) * smoothstep(-0.100000001490116119384765625, 0.300000011920928955078125, _452.moonDirection.y);
            color += (((_452.moonColor.xyz * moonProximity) * 0.07999999821186065673828125) * _484.moonTintStrength);
            float globalMoonTint = smoothstep(-0.100000001490116119384765625, 0.5, _452.moonDirection.y);
            color = mix(color, _452.moonColor.xyz, float3((globalMoonTint * _484.moonTintStrength) * 0.0500000007450580596923828125));
        }
    }
    out.FragColor = float4(color, 1.0);
    return out;
}

