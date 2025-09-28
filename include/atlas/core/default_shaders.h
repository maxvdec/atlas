// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* DEPTH_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection; // the light space matrix
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

static const char* SKYBOX_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}

)";

static const char* EMPTY_FRAG = R"(
#version 330

void main() {}

)";

static const char* COLOR_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec4 vertexColor;

void main() {
    FragColor = vertexColor;
}

)";

static const char* PARTICLE_FRAG = R"(
#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 FragColor;
uniform sampler2D particleTexture;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(particleTexture, fragTexCoord);
        if (texColor.a < 0.01) discard;
        FragColor = texColor * fragColor;
    } else {
        vec2 center = vec2(0.5, 0.5);
        float dist = distance(fragTexCoord, center);
        
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        
        FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
        
        if (FragColor.a < 0.01) discard;
    }
}
)";

static const char* AMBIENT_PARTICLE_FRAG = R"(

)";

static const char* FULLSCREEN_FRAG = R"(
#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

const int TEXTURE_COLOR = 0;
const int TEXTURE_DEPTH = 3;

uniform sampler2D Texture;
uniform int TextureType;

void main() {
    if (TextureType == TEXTURE_COLOR) {
        vec3 col = texture(Texture, TexCoord).rgb;
        FragColor = vec4(col, 1.0);
    } else if (TextureType == TEXTURE_DEPTH) {
        float depth = texture(Texture, TexCoord).r;
        FragColor = vec4(vec3(depth), 1.0);
    }
}

)";

static const char* MAIN_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;

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
};

// ----- Uniforms -----
uniform sampler2D textures[16];
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
            color += texture(textures[i], TexCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
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
    vec2 texelSize = 1.0 / textureSize(textures[shadowParam.textureIndex], 0);
    
    float distance = length(cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));
    
    int sampleCount = 0;
    for(int x = -kernelSize; x <= kernelSize; ++x) {
        for(int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = texture(textures[shadowParam.textureIndex], 
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
    float closestDepth = texture(textures[shadowParam.textureIndex], projCoords.xy).r;

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

// ----- Main -----
void main() {
    vec4 baseColor;

    if (useTexture && !useColor)
        baseColor = enableTextures(TEXTURE_COLOR);
    else if (useTexture && useColor)
        baseColor = enableTextures(TEXTURE_COLOR) * outColor;
    else if (!useTexture && useColor)
        baseColor = outColor;
    else
        baseColor = vec4(1.0);

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPosition - FragPos);

    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * material.ambient;
    vec3 directionalLights = calcAllDirectionalLights(norm, viewDir);
    vec3 pointLights = calcAllPointLights(norm, FragPos, viewDir);
    vec3 spotLightsContrib = calcAllSpotLights(norm, FragPos, viewDir);

    vec3 lightContribution = directionalLights + pointLights + spotLightsContrib;

    float shadow = calculateAllShadows();

    vec3 finalColor = (ambient + (1.0 - shadow) * lightContribution) * baseColor.rgb;

    FragColor = vec4(finalColor, baseColor.a);

    if (FragColor.a < 0.1)
        discard;
}

)";

static const char* TEXTURE_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 outColor;
out vec2 TexCoord; 

void main() {
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    outColor = aColor;
}

)";

static const char* DEBUG_FRAG = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}

)";

static const char* PARTICLE_VERT = R"(
#version 330 core

// Quad vertex attributes
layout(location = 0) in vec3 quadVertex; // Local quad position (-0.5 to 0.5)
layout(location = 1) in vec2 texCoord;   // Texture coordinates

// Instance attributes (per particle)
layout(location = 2) in vec3 particlePos;   // World position
layout(location = 3) in vec4 particleColor; // Color and alpha
layout(location = 4) in float particleSize; // Size in world units

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform bool isAmbient;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    if (isAmbient) {
        vec4 viewParticlePos = view * vec4(particlePos, 1.0);

        vec3 viewPosition =
            viewParticlePos.xyz +
            vec3(quadVertex.x * particleSize, quadVertex.y * particleSize, 0.0);

        gl_Position = projection * model * vec4(viewPosition, 1.0);
    } else {
        vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);

        vec3 worldPosition = particlePos +
                             (quadVertex.x * cameraRight * particleSize) +
                             (quadVertex.y * cameraUp * particleSize);

        gl_Position = projection * view * vec4(worldPosition, 1.0);
    }

    fragTexCoord = texCoord;
    fragColor = particleColor;
}
)";

static const char* COLOR_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(aPos, 1.0);
    vertexColor = aColor;
}

)";

static const char* SKYBOX_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; 
}  

)";

static const char* DEBUG_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

)";

static const char* TEXTURE_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;

uniform sampler2D textures[16];

uniform bool useTexture;
uniform bool onlyTexture;
uniform int textureCount;

vec4 calculateAllTextures() {
    vec4 color = vec4(0.0);

    for (int i = 0; i <= textureCount; i++) {
        color += texture(textures[i], TexCoord);
    }

    color /= float(textureCount + 1); 

    return color;
}

void main() {
    if (onlyTexture) {
        FragColor = calculateAllTextures(); 
        return;
    }

    if (useTexture) {
        FragColor = calculateAllTextures() * outColor;
    } else {
        FragColor = outColor;
    }
}

)";

static const char* MAIN_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 outColor;
out vec2 TexCoord; 
out vec3 Normal;
out vec3 FragPos;

void main() {
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    outColor = aColor;
}

)";

static const char* FULLSCREEN_VERT = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}

)";

static const char* AMBIENT_PARTICLE_VERT = R"(

)";

#endif // ATLAS_GENERATED_SHADERS_H
