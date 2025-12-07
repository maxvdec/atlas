#version 450
layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in float Height;
layout(location = 3) in vec4 FragPosLightSpace;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

layout(set = 2, binding = 0) uniform sampler2D heightMap;
layout(set = 2, binding = 1) uniform sampler2D moistureMap;
layout(set = 2, binding = 2) uniform sampler2D temperatureMap;
layout(set = 2, binding = 3) uniform sampler2D shadowMap;

layout(set = 2, binding = 4) uniform sampler2D texture0;
layout(set = 2, binding = 5) uniform sampler2D texture1;
layout(set = 2, binding = 6) uniform sampler2D texture2;
layout(set = 2, binding = 7) uniform sampler2D texture3;
layout(set = 2, binding = 8) uniform sampler2D texture4;
layout(set = 2, binding = 9) uniform sampler2D texture5;
layout(set = 2, binding = 10) uniform sampler2D texture6;
layout(set = 2, binding = 11) uniform sampler2D texture7;
layout(set = 2, binding = 12) uniform sampler2D texture8;
layout(set = 2, binding = 13) uniform sampler2D texture9;
layout(set = 2, binding = 14) uniform sampler2D texture10;
layout(set = 2, binding = 15) uniform sampler2D texture11;

layout(push_constant) uniform PushConstants {
    bool isFromMap;
    vec4 directionalColor;
    float directionalIntensity;
    bool hasLight;
    bool useShadowMap;
    vec3 lightDir;
    vec3 viewDir;
    float ambientStrength;
    float shadowBias;
    int biomesCount;
    float diffuseStrength;
    float specularStrength;
} ;

layout(set = 1, binding = 0) uniform TerrainParameters {
    float maxPeak;
    float seaLevel;
};

struct Biome {
    int id;
    vec4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

layout(std430, set = 3, binding = 0) buffer BiomeBuffer {
    Biome biomes[];
};

vec4 sampleBiomeTexture(int id, vec2 uv) {
    if (id == 0) return texture(texture0, uv);
    if (id == 1) return texture(texture1, uv);
    if (id == 2) return texture(texture2, uv);
    if (id == 3) return texture(texture3, uv);
    if (id == 4) return texture(texture4, uv);
    if (id == 5) return texture(texture5, uv);
    if (id == 6) return texture(texture6, uv);
    if (id == 7) return texture(texture7, uv);
    if (id == 8) return texture(texture8, uv);
    if (id == 9) return texture(texture9, uv);
    if (id == 10) return texture(texture10, uv);
    if (id == 11) return texture(texture11, uv);
    return vec4(1,0,1,1);
}

vec4 triplanarBlend(int idx, vec3 normal, vec3 worldPos, float scale) {
    vec3 blend = abs(normal);
    blend = (blend - 0.2) * 7.0;
    blend = clamp(blend, 0.0, 1.0);
    blend /= (blend.x + blend.y + blend.z);

    vec4 xProj = sampleBiomeTexture(idx, worldPos.yz * scale);
    vec4 yProj = sampleBiomeTexture(idx, worldPos.xz * scale);
    vec4 zProj = sampleBiomeTexture(idx, worldPos.xy * scale);

    return xProj * blend.x + yProj * blend.y + zProj * blend.z;
}

vec3 calculateNormal(sampler2D heightMap, vec2 texCoord, float heightScale)
{
    float h = texture(heightMap, texCoord).r * heightScale;

    float dx = dFdx(h);
    float dy = dFdy(h);

    vec3 n = normalize(vec3(-dx, 1.0, -dy));
    return n;
}

float smoothStepRange(float value, float minV, float maxV) {
    return smoothstep(minV, maxV, value);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.1);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 acesToneMapping(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    if (biomesCount <= 0) {
        FragColor = vec4(vec3((Height + seaLevel) / maxPeak), 1.0);
        return;
    }

    float h = isFromMap ? texture(heightMap, TexCoord).r * 255.0 : (Height + seaLevel) / maxPeak * 255.0;
    float m = texture(moistureMap, TexCoord).r * 255.0;
    float t = texture(temperatureMap, TexCoord).r * 255.0;

    float texelSize = 1.0 / textureSize(heightMap, 0).x;
    float heightScale = 64.0;
    vec3 normal = calculateNormal(heightMap, TexCoord, heightScale);

    // === BIOME BLENDING ===
    vec4 baseColor = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < biomesCount; i++) {
        Biome b = biomes[i]; 

        float hW = (b.minHeight < 0.0 && b.maxHeight < 0.0) ? 1.0 : smoothStepRange(h, b.minHeight, b.maxHeight);
        float mW = (b.minMoisture < 0.0 && b.maxMoisture < 0.0) ? 1.0 : smoothStepRange(m, b.minMoisture, b.maxMoisture);
        float tW = (b.minTemperature < 0.0 && b.maxTemperature < 0.0) ? 1.0 : smoothStepRange(t, b.minTemperature, b.maxTemperature);

        float weight = (1.0 - hW) * mW * tW;
        if (weight > 0.001) {
            vec4 texColor = (b.useTexture == 1)
                ? triplanarBlend(i, normal, FragPos, 0.1)
                : b.tintColor;
            baseColor += texColor * weight;
            totalWeight += weight;
        }
    }

    baseColor /= max(totalWeight, 0.001);

    float detail = texture(heightMap, TexCoord * 64.0).r * 0.1 + 0.9;
    baseColor.rgb *= detail;

    vec3 finalColor;
    
    if (hasLight) {
        vec3 L = normalize(-lightDir);
        vec3 N = normalize(normal);
        vec3 V = normalize(viewDir);
        
        vec3 ambient = ambientStrength * baseColor.rgb;
        
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diffuseStrength * diff * directionalColor.rgb * directionalIntensity * baseColor.rgb;
        
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        vec3 specular = specularStrength * spec * directionalColor.rgb * directionalIntensity;
        
        float shadow = 0.0;
        if (useShadowMap) {
            shadow = calculateShadow(FragPosLightSpace, N);
        }
        
        finalColor = ambient + (1.0 - shadow) * (diffuse + specular);
        
    } else {
        finalColor = ambientStrength * baseColor.rgb;
    }
    
    FragColor = vec4(acesToneMapping(finalColor), 1.0);
    BrightColor = vec4(0.0);
}