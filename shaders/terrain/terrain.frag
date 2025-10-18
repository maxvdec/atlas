#version 410 core
in vec3 FragPos;
in float Height;
in vec2 TexCoord;

uniform sampler2D biomesMap;
uniform sampler2D heightMap;

struct Biome {
    int id;
    vec4 tintColor;
    int useTexture;
    int textureId;
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

void main() {
    if (biomesCount <= 0) {
        FragColor = vec4(vec3((Height + 16) / 64.0f), 1.0);
        return;
    }
    
    float biomeData = texture(biomesMap, TexCoord).r;
    
    int biomeId = int(floor(biomeData * float(biomesCount)));
    biomeId = clamp(biomeId, 0, biomesCount - 1);
    
    vec2 texelSize = 1.0 / vec2(textureSize(biomesMap, 0));
    float blendRadius = 3.0;
    
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    
    for (float dy = -blendRadius; dy <= blendRadius; dy += 1.0) {
        for (float dx = -blendRadius; dx <= blendRadius; dx += 1.0) {
            vec2 offset = vec2(dx, dy) * texelSize;
            float neighborBiomeData = texture(biomesMap, TexCoord + offset).r;
            int neighborBiomeId = int(floor(neighborBiomeData * float(biomesCount)));
            neighborBiomeId = clamp(neighborBiomeId, 0, biomesCount - 1);
            
            float dist = length(vec2(dx, dy));
            float weight = exp(-dist * dist / (blendRadius * blendRadius));
            
            vec4 biomeColor = getBiomeColor(neighborBiomeId, TexCoord);
            
            color += biomeColor * weight;
            totalWeight += weight;
        }
    }
    
    if (totalWeight > 0.0) {
        FragColor = color / totalWeight;
    } else {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green error color
    }
    
    BrightColor = vec4(0.0);
}