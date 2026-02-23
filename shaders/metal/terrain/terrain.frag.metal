#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct PushConstants
{
    uint isFromMap;
    float4 directionalColor;
    float directionalIntensity;
    uint hasLight;
    uint useShadowMap;
    float3 lightDir;
    packed_float3 viewDir;
    float ambientStrength;
    float shadowBias;
    int biomesCount;
    float diffuseStrength;
    float specularStrength;
};

struct TerrainParameters
{
    float maxPeak;
    float seaLevel;
};

struct Biome
{
    int id;
    float4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

struct Biome_1
{
    int id;
    float4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

struct BiomeBuffer
{
    Biome_1 biomes[1];
};

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float3 FragPos [[user(locn1)]];
    float Height [[user(locn2)]];
    float4 FragPosLightSpace [[user(locn3)]];
};

static inline __attribute__((always_inline))
float3 calculateNormal(texture2d<float> heightMap, sampler heightMapSmplr, thread const float2& texCoord, thread const float& heightScale)
{
    float h = heightMap.sample(heightMapSmplr, texCoord).x * heightScale;
    float dx = dfdx(h);
    float dy = dfdy(h);
    float3 n = fast::normalize(float3(-dx, 1.0, -dy));
    return n;
}

static inline __attribute__((always_inline))
float smoothStepRange(thread const float& value, thread const float& minV, thread const float& maxV)
{
    return smoothstep(minV, maxV, value);
}

static inline __attribute__((always_inline))
float4 sampleBiomeTexture(thread const int& id, thread const float2& uv, texture2d<float> texture0, sampler texture0Smplr, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr)
{
    if (id == 0)
    {
        return texture0.sample(texture0Smplr, uv);
    }
    if (id == 1)
    {
        return texture1.sample(texture1Smplr, uv);
    }
    if (id == 2)
    {
        return texture2.sample(texture2Smplr, uv);
    }
    if (id == 3)
    {
        return texture3.sample(texture3Smplr, uv);
    }
    if (id == 4)
    {
        return texture4.sample(texture4Smplr, uv);
    }
    if (id == 5)
    {
        return texture5.sample(texture5Smplr, uv);
    }
    if (id == 6)
    {
        return texture6.sample(texture6Smplr, uv);
    }
    if (id == 7)
    {
        return texture7.sample(texture7Smplr, uv);
    }
    if (id == 8)
    {
        return texture8.sample(texture8Smplr, uv);
    }
    if (id == 9)
    {
        return texture9.sample(texture9Smplr, uv);
    }
    if (id == 10)
    {
        return texture10.sample(texture10Smplr, uv);
    }
    if (id == 11)
    {
        return texture11.sample(texture11Smplr, uv);
    }
    return float4(1.0, 0.0, 1.0, 1.0);
}

static inline __attribute__((always_inline))
float4 triplanarBlend(thread const int& idx, thread const float3& normal, thread const float3& worldPos, thread const float& scale, texture2d<float> texture0, sampler texture0Smplr, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr)
{
    float3 blend = abs(normal);
    blend = (blend - float3(0.20000000298023223876953125)) * 7.0;
    blend = fast::clamp(blend, float3(0.0), float3(1.0));
    blend /= float3((blend.x + blend.y) + blend.z);
    int param = idx;
    float2 param_1 = worldPos.yz * scale;
    float4 xProj = sampleBiomeTexture(param, param_1, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    int param_2 = idx;
    float2 param_3 = worldPos.xz * scale;
    float4 yProj = sampleBiomeTexture(param_2, param_3, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    int param_4 = idx;
    float2 param_5 = worldPos.xy * scale;
    float4 zProj = sampleBiomeTexture(param_4, param_5, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    return ((xProj * blend.x) + (yProj * blend.y)) + (zProj * blend.z);
}

static inline __attribute__((always_inline))
float calculateShadow(thread const float4& fragPosLightSpace, thread const float3& normal, constant PushConstants& _331, texture2d<float> shadowMap, sampler shadowMapSmplr)
{
    float3 projCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    projCoords = (projCoords * 0.5) + float3(0.5);
    bool _293 = projCoords.z > 1.0;
    bool _300;
    if (!_293)
    {
        _300 = projCoords.x < 0.0;
    }
    else
    {
        _300 = _293;
    }
    bool _307;
    if (!_300)
    {
        _307 = projCoords.x > 1.0;
    }
    else
    {
        _307 = _300;
    }
    bool _314;
    if (!_307)
    {
        _314 = projCoords.y < 0.0;
    }
    else
    {
        _314 = _307;
    }
    bool _321;
    if (!_314)
    {
        _321 = projCoords.y > 1.0;
    }
    else
    {
        _321 = _314;
    }
    if (_321)
    {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    float bias0 = fast::max(_331.shadowBias * (1.0 - dot(normal, _331.lightDir)), _331.shadowBias * 0.100000001490116119384765625);
    float shadow = 0.0;
    float2 texelSize = float2(1.0) / float2(int2(shadowMap.get_width(), shadowMap.get_height()));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = shadowMap.sample(shadowMapSmplr, (projCoords.xy + (float2(float(x), float(y)) * texelSize))).x;
            shadow += float((currentDepth - bias0) > pcfDepth);
        }
    }
    shadow /= 9.0;
    return shadow;
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread const float3& color)
{
    return fast::clamp((color * ((color * 2.5099999904632568359375) + float3(0.02999999932944774627685546875))) / ((color * ((color * 2.4300000667572021484375) + float3(0.589999973773956298828125))) + float3(0.14000000059604644775390625)), float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant PushConstants& _331 [[buffer(0)]], constant TerrainParameters& _444 [[buffer(1)]], device BiomeBuffer& _534 [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], texture2d<float> texture2 [[texture(2)]], texture2d<float> texture3 [[texture(3)]], texture2d<float> texture4 [[texture(4)]], texture2d<float> texture5 [[texture(5)]], texture2d<float> texture6 [[texture(6)]], texture2d<float> texture7 [[texture(7)]], texture2d<float> texture8 [[texture(8)]], texture2d<float> texture9 [[texture(9)]], texture2d<float> texture10 [[texture(10)]], texture2d<float> texture11 [[texture(11)]], texture2d<float> shadowMap [[texture(12)]], texture2d<float> heightMap [[texture(13)]], texture2d<float> moistureMap [[texture(14)]], texture2d<float> temperatureMap [[texture(15)]], sampler texture0Smplr [[sampler(0)]], sampler texture1Smplr [[sampler(1)]], sampler texture2Smplr [[sampler(2)]], sampler texture3Smplr [[sampler(3)]], sampler texture4Smplr [[sampler(4)]], sampler texture5Smplr [[sampler(5)]], sampler texture6Smplr [[sampler(6)]], sampler texture7Smplr [[sampler(7)]], sampler texture8Smplr [[sampler(8)]], sampler texture9Smplr [[sampler(9)]], sampler texture10Smplr [[sampler(10)]], sampler texture11Smplr [[sampler(11)]], sampler shadowMapSmplr [[sampler(12)]], sampler heightMapSmplr [[sampler(13)]], sampler moistureMapSmplr [[sampler(14)]], sampler temperatureMapSmplr [[sampler(15)]])
{
    main0_out out = {};
    if (_331.biomesCount <= 0)
    {
        out.FragColor = float4(float3((in.Height + _444.seaLevel) / _444.maxPeak), 1.0);
        return out;
    }
    float _463;
    if (_331.isFromMap != 0u)
    {
        _463 = heightMap.sample(heightMapSmplr, in.TexCoord).x * 255.0;
    }
    else
    {
        _463 = ((in.Height + _444.seaLevel) / _444.maxPeak) * 255.0;
    }
    float h = _463;
    float m = moistureMap.sample(moistureMapSmplr, in.TexCoord).x * 255.0;
    float t = temperatureMap.sample(temperatureMapSmplr, in.TexCoord).x * 255.0;
    float texelSize = 1.0 / float(int2(heightMap.get_width(), heightMap.get_height()).x);
    float heightScale = 64.0;
    float2 param = in.TexCoord;
    float param_1 = heightScale;
    float3 normal = calculateNormal(heightMap, heightMapSmplr, param, param_1);
    float4 baseColor = float4(0.0);
    float totalWeight = 0.0;
    float _550;
    float _574;
    float _598;
    float4 _628;
    for (int i = 0; i < _331.biomesCount; i++)
    {
        Biome _539;
        _539.id = _534.biomes[i].id;
        _539.tintColor = _534.biomes[i].tintColor;
        _539.useTexture = _534.biomes[i].useTexture;
        _539.textureId = _534.biomes[i].textureId;
        _539.minHeight = _534.biomes[i].minHeight;
        _539.maxHeight = _534.biomes[i].maxHeight;
        _539.minMoisture = _534.biomes[i].minMoisture;
        _539.maxMoisture = _534.biomes[i].maxMoisture;
        _539.minTemperature = _534.biomes[i].minTemperature;
        _539.maxTemperature = _534.biomes[i].maxTemperature;
        Biome b = _539;
        bool _543 = b.minHeight < 0.0;
        bool _549;
        if (_543)
        {
            _549 = b.maxHeight < 0.0;
        }
        else
        {
            _549 = _543;
        }
        if (_549)
        {
            _550 = 1.0;
        }
        else
        {
            float param_2 = h;
            float param_3 = b.minHeight;
            float param_4 = b.maxHeight;
            _550 = smoothStepRange(param_2, param_3, param_4);
        }
        float hW = _550;
        bool _567 = b.minMoisture < 0.0;
        bool _573;
        if (_567)
        {
            _573 = b.maxMoisture < 0.0;
        }
        else
        {
            _573 = _567;
        }
        if (_573)
        {
            _574 = 1.0;
        }
        else
        {
            float param_5 = m;
            float param_6 = b.minMoisture;
            float param_7 = b.maxMoisture;
            _574 = smoothStepRange(param_5, param_6, param_7);
        }
        float mW = _574;
        bool _591 = b.minTemperature < 0.0;
        bool _597;
        if (_591)
        {
            _597 = b.maxTemperature < 0.0;
        }
        else
        {
            _597 = _591;
        }
        if (_597)
        {
            _598 = 1.0;
        }
        else
        {
            float param_8 = t;
            float param_9 = b.minTemperature;
            float param_10 = b.maxTemperature;
            _598 = smoothStepRange(param_8, param_9, param_10);
        }
        float tW = _598;
        float weight = ((1.0 - hW) * mW) * tW;
        if (weight > 0.001000000047497451305389404296875)
        {
            if (b.useTexture == 1)
            {
                int param_11 = i;
                float3 param_12 = normal;
                float3 param_13 = in.FragPos;
                float param_14 = 0.100000001490116119384765625;
                _628 = triplanarBlend(param_11, param_12, param_13, param_14, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
            }
            else
            {
                _628 = b.tintColor;
            }
            float4 texColor = _628;
            baseColor += (texColor * weight);
            totalWeight += weight;
        }
    }
    baseColor /= float4(fast::max(totalWeight, 0.001000000047497451305389404296875));
    float detail = (heightMap.sample(heightMapSmplr, (in.TexCoord * 64.0)).x * 0.100000001490116119384765625) + 0.89999997615814208984375;
    float4 _670 = baseColor;
    float3 _672 = _670.xyz * detail;
    baseColor.x = _672.x;
    baseColor.y = _672.y;
    baseColor.z = _672.z;
    float3 finalColor;
    if (_331.hasLight != 0u)
    {
        float3 L = fast::normalize(-_331.lightDir);
        float3 N = fast::normalize(normal);
        float3 V = fast::normalize(float3(_331.viewDir));
        float3 ambient = baseColor.xyz * _331.ambientStrength;
        float diff = fast::max(dot(N, L), 0.0);
        float3 diffuse = ((_331.directionalColor.xyz * (_331.diffuseStrength * diff)) * _331.directionalIntensity) * baseColor.xyz;
        float3 H = fast::normalize(L + V);
        float spec = powr(fast::max(dot(N, H), 0.0), 32.0);
        float3 specular = (_331.directionalColor.xyz * (_331.specularStrength * spec)) * _331.directionalIntensity;
        float shadow = 0.0;
        if (_331.useShadowMap != 0u)
        {
            float4 param_15 = in.FragPosLightSpace;
            float3 param_16 = N;
            shadow = calculateShadow(param_15, param_16, _331, shadowMap, shadowMapSmplr);
        }
        finalColor = ambient + ((diffuse + specular) * (1.0 - shadow));
    }
    else
    {
        finalColor = baseColor.xyz * _331.ambientStrength;
    }
    float3 param_17 = finalColor;
    out.FragColor = float4(acesToneMapping(param_17), 1.0);
    out.BrightColor = float4(0.0);
    return out;
}

