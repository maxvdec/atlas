#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 gMaterial;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float reflectivity;
};

// Textures
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

// Uniforms
uniform int textureTypes[16];
uniform int textureCount;
uniform Material material;
uniform vec3 cameraPosition;
uniform bool useTexture;
uniform bool useColor;

vec2 texCoord;

vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) {
            if (i == 0) color += texture(texture1, texCoord);
            else if (i == 1) color += texture(texture2, texCoord);
            else if (i == 2) color += texture(texture3, texCoord);
            else if (i == 3) color += texture(texture4, texCoord);
            else if (i == 4) color += texture(texture5, texCoord);
            else if (i == 5) color += texture(texture6, texCoord);
            else if (i == 6) color += texture(texture7, texCoord);
            else if (i == 7) color += texture(texture8, texCoord);
            else if (i == 8) color += texture(texture9, texCoord);
            else if (i == 9) color += texture(texture10, texCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    else if (textureIndex == 5) return texture(texture6, uv);
    else if (textureIndex == 6) return texture(texture7, uv);
    else if (textureIndex == 7) return texture(texture8, uv);
    else if (textureIndex == 8) return texture(texture9, uv);
    else if (textureIndex == 9) return texture(texture10, uv);
    return vec4(0.0);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    vec2 P = viewDir.xy * 0.1;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) return texCoords;
    
    float currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return currentTexCoords;
}

void main() {
    texCoord = TexCoord;
    
    vec3 tangentViewDir = normalize((TBN * cameraPosition) - (TBN * FragPos));
    texCoord = parallaxMapping(texCoord, tangentViewDir);
    
    if (texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0)
        discard;
    
    vec4 baseColor;
    if (useTexture && !useColor)
        baseColor = enableTextures(TEXTURE_COLOR);
    else if (useTexture && useColor)
        baseColor = enableTextures(TEXTURE_COLOR) * outColor;
    else if (!useTexture && useColor)
        baseColor = outColor;
    else
        baseColor = vec4(1.0);
    
    if (baseColor.a < 0.1)
        discard;
    
    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 normal;
    if (normTexture.r != -1.0 || normTexture.g != -1.0 || normTexture.b != -1.0) {
        normal = normalize(normTexture.rgb * 2.0 - 1.0);
        normal = normalize(TBN * normal);
    } else {
        normal = normalize(Normal);
    }
    
    vec4 specTex = enableTextures(TEXTURE_SPECULAR);
    vec3 specColor = material.specular;
    if (specTex.r != -1.0 || specTex.g != -1.0 || specTex.b != -1.0) {
        specColor *= specTex.rgb;
    }
    
    gPosition = vec4(FragPos, gl_FragCoord.z);
    
    gNormal = vec4(normal, 1.0);

    float specIntensity = (specColor.r + specColor.g + specColor.b) / 3.0;
    gAlbedoSpec = vec4(baseColor.rgb, specIntensity);
    
    gMaterial = vec4(
        material.shininess / 256.0,  
        material.reflectivity,
        0.0,  
        1.0   
    );
}