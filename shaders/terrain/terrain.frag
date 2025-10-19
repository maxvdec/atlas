#version 410 core

in vec3 FragPos;
in float Height;
in vec2 TexCoord;
in vec4 FragPosLightSpace;  

uniform sampler2D heightMap;
uniform sampler2D moistureMap;
uniform sampler2D temperatureMap;
uniform sampler2D shadowMap;  

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;
uniform sampler2D texture11;

uniform float maxPeak;
uniform float seaLevel;
uniform bool isFromMap;

uniform vec4 directionalColor;
uniform float directionalIntensity;

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

uniform Biome biomes[12];
uniform int biomesCount;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform bool hasLight = false;
uniform bool useShadowMap = false;
uniform vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
uniform vec3 viewDir  = normalize(vec3(0.0, 1.0, 1.0));
uniform float ambientStrength = 0.6;  
const float diffuseStrength = 0.8;
const float specularStrength = 0.15;
uniform float shadowBias = 0.005;

// === Textures ===
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

// === Triplanar mapping ===
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

// === Normal from height ===
vec3 calculateNormal(sampler2D heightMap, vec2 texCoord, float heightScale)
{
    float h = texture(heightMap, texCoord).r * heightScale;

    float dx = dFdx(h);
    float dy = dFdy(h);

    vec3 n = normalize(vec3(-dx, 1.0, -dy));
    return n;
}


// === Smooth biome blend ===
float smoothStepRange(float value, float minV, float maxV) {
    return smoothstep(minV, maxV, value);
}

// === Sharp shadow calculation ===
float calculateShadow(vec4 fragPosLightSpace, vec3 normal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.1);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
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

    finalColor = ambientStrength * baseColor.rgb;
    if (hasLight) {
        finalColor = baseColor.rgb * directionalColor.rgb * (directionalIntensity - 0.2);
    }
    FragColor = vec4(acesToneMapping(finalColor), 1.0);
    BrightColor = vec4(0.0);
    return;
}