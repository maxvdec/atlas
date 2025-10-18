#version 410 core
in vec3 FragPos;
in float Height;
in vec2 TexCoord;

uniform sampler2D biomesMap;
uniform sampler2D heightMap;
uniform sampler2D moistureMap;
uniform sampler2D temperatureMap;

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

uniform Biome biomes[12];
uniform int biomesCount;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

const vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); 
const float ambientStrength = 0.4; 
const float diffuseStrength = 0.6; 

vec4 sampleBiomeTexture(int biomeIdx, vec2 uv) {
    vec4 result = vec4(1.0, 0.0, 1.0, 1.0); 
    
    if (biomeIdx == 0) result = texture(texture0, uv);
    if (biomeIdx == 1) result = texture(texture1, uv);
    if (biomeIdx == 2) result = texture(texture2, uv);
    if (biomeIdx == 3) result = texture(texture3, uv);
    if (biomeIdx == 4) result = texture(texture4, uv);
    if (biomeIdx == 5) result = texture(texture5, uv);
    if (biomeIdx == 6) result = texture(texture6, uv);
    if (biomeIdx == 7) result = texture(texture7, uv);
    if (biomeIdx == 8) result = texture(texture8, uv);
    if (biomeIdx == 9) result = texture(texture9, uv);
    if (biomeIdx == 10) result = texture(texture10, uv);
    if (biomeIdx == 11) result = texture(texture11, uv);
    
    return result;
}

vec4 getBiomeColor(int biomeIdx, vec2 uv) {
    if (biomeIdx < 0 || biomeIdx >= biomesCount) {
        return vec4(1.0, 0.0, 0.0, 1.0); 
    }
    
    Biome b = biomes[biomeIdx];
    if (b.useTexture == 1) {
        vec4 texColor = sampleBiomeTexture(b.textureId, uv);
        return texColor;
    } else {
        return b.tintColor;
    }
}

vec3 calculateNormal(sampler2D heightMap, vec2 texCoord, float texelSize) {
    float heightL = texture(heightMap, texCoord + vec2(-texelSize, 0.0)).r;
    float heightR = texture(heightMap, texCoord + vec2(texelSize, 0.0)).r;
    float heightD = texture(heightMap, texCoord + vec2(0.0, -texelSize)).r;
    float heightU = texture(heightMap, texCoord + vec2(0.0, texelSize)).r;
    
    vec3 normal;
    normal.x = (heightL - heightR);
    normal.y = 2.0 * texelSize; 
    normal.z = (heightD - heightU);
    
    return normalize(normal);
}

float calculateAO(sampler2D heightMap, vec2 texCoord, float texelSize) {
    float centerHeight = texture(heightMap, texCoord).r;
    float ao = 0.0;
    int samples = 0;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            float sampleHeight = texture(heightMap, texCoord + offset).r;
            ao += max(0.0, centerHeight - sampleHeight);
            samples++;
        }
    }
    
    ao /= float(samples);
    return 1.0 - clamp(ao * 8.0, 0.0, 0.7); 
}

void main() {
    if (biomesCount <= 0) {
        FragColor = vec4(vec3((Height + 16) / 64.0f), 1.0);
        return;
    }
    
    float moisture = texture(moistureMap, TexCoord).r * 255;
    float temperature = texture(temperatureMap, TexCoord).r * 255;
    float height = texture(heightMap, TexCoord).r * 255;
    
    vec4 baseColor = vec4(0.0);
    
    for (int i = 0; i < biomesCount; i++) {
        Biome b = biomes[i];
        bool heightInRange = (b.minHeight < 0.0f || Height * 255.0 >= b.minHeight) &&
                           (b.maxHeight < 0.0f || Height * 255.0 < b.maxHeight);
        bool moistureInRange = (b.minMoisture < 0.0f || moisture >= b.minMoisture) &&
                               (b.maxMoisture < 0.0f || moisture < b.maxMoisture);
        bool temperatureInRange = (b.minTemperature < 0.0f || temperature >= b.minTemperature) &&
                                  (b.maxTemperature < 0.0f || temperature < b.maxTemperature);
        
        if (heightInRange && moistureInRange && temperatureInRange) {
            baseColor = getBiomeColor(b.id, TexCoord * 10.0);
            break;
        }
    }
    
    float texelSize = 1.0 / textureSize(heightMap, 0).x; 
    vec3 normal = calculateNormal(heightMap, TexCoord, texelSize);
    
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    float ao = calculateAO(heightMap, TexCoord, texelSize * 2.0);
    
    float lighting = ambientStrength + diffuseStrength * diffuse;
    lighting *= ao; 
    
    FragColor = vec4(baseColor.rgb * lighting, baseColor.a);
}