#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_DEPTH_CUBE = 4;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;

vec2 texCoord;

// ----- Structures -----
struct AmbientLight {
    vec4 color;
    float intensity;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float reflectivity;
};

struct DirectionalLight {
    vec3 direction;  
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 diffuse;
    vec3 specular;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    vec3 lightPos;
    bool isPointLight;
};

// ----- Textures -----
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
uniform samplerCube skybox;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;

// ----- Uniforms -----
uniform int textureTypes[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;

uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;

uniform PointLight pointLights[32];
uniform int pointLightCount;

uniform SpotLight spotlights[32];
uniform int spotlightCount;

uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;

// ----- Helper Functions -----
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

vec4 enableCubeMaps(int type, vec3 direction) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (type == i + 10) { 
            if (i == 0) color += texture(cubeMap1, direction);
            else if (i == 1) color += texture(cubeMap2, direction);
            else if (i == 2) color += texture(cubeMap3, direction);
            else if (i == 3) color += texture(cubeMap4, direction);
            else if (i == 4) color += texture(cubeMap5, direction);
            count++; 
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 sampleCubeTextureAt(int textureIndex, vec3 direction) {
    if (textureIndex == 0) return texture(cubeMap1, direction);
    else if (textureIndex == 1) return texture(cubeMap2, direction);
    else if (textureIndex == 2) return texture(cubeMap3, direction);
    else if (textureIndex == 3) return texture(cubeMap4, direction);
    else if (textureIndex == 4) return texture(cubeMap5, direction);
    return vec4(0.0);
}

vec2 getTextureDimensions(int textureIndex) {
    if (textureIndex == 0) return vec2(textureSize(texture1, 0));
    else if (textureIndex == 1) return vec2(textureSize(texture2, 0));
    else if (textureIndex == 2) return vec2(textureSize(texture3, 0));
    else if (textureIndex == 3) return vec2(textureSize(texture4, 0));
    else if (textureIndex == 4) return vec2(textureSize(texture5, 0));
    else if (textureIndex == 5) return vec2(textureSize(texture6, 0));
    else if (textureIndex == 6) return vec2(textureSize(texture7, 0));
    else if (textureIndex == 7) return vec2(textureSize(texture8, 0));
    else if (textureIndex == 8) return vec2(textureSize(texture9, 0));
    else if (textureIndex == 9) return vec2(textureSize(texture10, 0));
    return vec2(0);
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

vec3 getSpecularColor() {
    vec4 specTex = enableTextures(TEXTURE_SPECULAR);
    vec3 specColor = material.specular;
    if (specTex.r != -1.0 || specTex.g != -1.0 || specTex.b != -1.0) {
        specColor *= specTex.rgb;
    }
    return specColor;
}

vec4 applyGammaCorrection(vec4 color, float gamma) {
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
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

// ----- Environment Mapping -----
vec4 getEnvironmentReflected(vec4 color) {
    vec3 I = normalize(FragPos - cameraPosition);
    vec3 R = reflect(I, normalize(Normal));
    return mix(color, vec4(texture(skybox, R).rgb, 1.0), material.reflectivity);
}

// ----- Directional Light -----
vec3 calcDirectionalDiffuse(DirectionalLight light, vec3 norm) {
    vec3 lightDir = normalize(-light.direction);  
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * light.diffuse;
}

vec3 calcDirectionalSpecular(DirectionalLight light, vec3 norm, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(-light.direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    return spec * specColor * light.specular;
}

vec3 calcAllDirectionalLights(vec3 norm, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < directionalLightCount; i++) {
        diffuseSum += calcDirectionalDiffuse(directionalLights[i], norm);
        specularSum += calcDirectionalSpecular(directionalLights[i], norm, viewDir, specColor, material.shininess);
    }

    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Point Light -----
vec3 calcPointDiffuse(PointLight light, vec3 norm, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * light.diffuse;
}

vec3 calcPointSpecular(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    return spec * specColor * light.specular;
}

float calcAttenuation(PointLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);
}

vec3 calcAllPointLights(vec3 norm, vec3 fragPos, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < pointLightCount; i++) {
        float attenuation = calcAttenuation(pointLights[i], fragPos);
        diffuseSum += calcPointDiffuse(pointLights[i], norm, fragPos) * attenuation;
        specularSum += calcPointSpecular(pointLights[i], norm, fragPos, viewDir, specColor, material.shininess) * attenuation;
    }
    
    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Spot Light -----
vec3 calcSpotDiffuse(SpotLight light, vec3 norm, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 spotDirection = normalize(light.direction);
    float theta = dot(lightDir, -spotDirection); 
    
    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);
    
    return diff * light.diffuse * intensity;
}


vec3 calcSpotSpecular(SpotLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    
    vec3 spotDirection = normalize(light.direction);
    float theta = dot(lightDir, -spotDirection);  
    
    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);
    
    return spec * specColor * light.specular * intensity;
}


float calcSpotAttenuation(SpotLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (1.0 + 0.09 * distance + 0.032 * distance);
}

vec3 calcAllSpotLights(vec3 norm, vec3 fragPos, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < spotlightCount; i++) {
        float attenuation = calcSpotAttenuation(spotlights[i], fragPos);
        diffuseSum += calcSpotDiffuse(spotlights[i], norm, fragPos) * attenuation;
        specularSum += calcSpotSpecular(spotlights[i], norm, fragPos, viewDir, specColor, material.shininess) * attenuation;
    }

    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Shadow Calculations -----
float calculateShadow(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0 || 
        projCoords.z > 1.0) {
        return 0.0;
    }
    
    float currentDepth = projCoords.z;
    
    vec3 lightDir = normalize(-directionalLights[0].direction); 
    vec3 normal = normalize(Normal);
    float biasValue = shadowParam.bias;
    float bias = max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);
    
    float distance = length(cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));
    
    int sampleCount = 0;
    for(int x = -kernelSize; x <= kernelSize; ++x) {
        for(int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = sampleTextureAt(shadowParam.textureIndex, 
                                   projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
            sampleCount++;
        }    
    }
    shadow /= float(sampleCount);
    
    return shadow;
}

float calculateShadowRaw(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0 || 
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float closestDepth = sampleTextureAt(shadowParam.textureIndex, projCoords.xy).r;

    return currentDepth > closestDepth ? 1.0 : 0.0;
}

float calculateAllShadows() {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        vec4 fragPosLightSpace = shadowParams[i].lightProjection * shadowParams[i].lightView * vec4(FragPos, 1.0);
        float shadow = calculateShadow(shadowParams[i], fragPosLightSpace);
        totalShadow = max(totalShadow, shadow);
    }
    return totalShadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos) {
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, fragToLight).r;
    closestDepth *= shadowParam.farPlane;

    float currentDepth = length(fragToLight);
    float bias = 0.05; 
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

float calculateAllPointShadows(vec3 fragPos) {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (shadowParams[i].isPointLight) {
            float shadow = calculatePointShadow(shadowParams[i], fragPos);
            totalShadow = max(totalShadow, shadow);
        }
    }
    return totalShadow;
}

// ----- Main -----
void main() {
    texCoord = TexCoord;
    vec4 baseColor;

    vec3 tangentViewDir = normalize((TBN * cameraPosition) - (TBN * FragPos));
    texCoord = parallaxMapping(texCoord, tangentViewDir);
    if(texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0)
        discard;

    if (useTexture && !useColor)
        baseColor = enableTextures(TEXTURE_COLOR);
    else if (useTexture && useColor)
        baseColor = enableTextures(TEXTURE_COLOR) * outColor;
    else if (!useTexture && useColor)
        baseColor = outColor;
    else
        baseColor = vec4(1.0);

    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 norm = vec3(0.0);
    if (normTexture.r != -1.0 || normTexture.g != -1.0 || normTexture.b != -1.0) {
        norm = normalize(normTexture.rgb * 2.0 - 1.0);
        norm = normalize(TBN * norm);
    } else {
        norm = normalize(Normal);
    }
    vec3 viewDir = normalize(cameraPosition - FragPos);

    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * material.ambient;
    float dirShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (!shadowParams[i].isPointLight) {
            vec4 fragPosLightSpace = shadowParams[i].lightProjection * 
                                     shadowParams[i].lightView * 
                                     vec4(FragPos, 1.0);
            dirShadow = max(dirShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
        }
    }
    
    float pointShadow = calculateAllPointShadows(FragPos);
    
    vec3 directionalLights = calcAllDirectionalLights(norm, viewDir) * (1.0 - dirShadow);
    vec3 pointLights = calcAllPointLights(norm, FragPos, viewDir) * (1.0 - pointShadow);
    vec3 spotLightsContrib = calcAllSpotLights(norm, FragPos, viewDir);  

    vec3 finalColor = (ambient + directionalLights + pointLights + spotLightsContrib) * baseColor.rgb;

    FragColor = vec4(finalColor, baseColor.a);
    FragColor = getEnvironmentReflected(FragColor);

    if (FragColor.a < 0.1)
        discard;
}
