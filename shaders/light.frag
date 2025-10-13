#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterial;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;
uniform samplerCube skybox;

struct AmbientLight {
    vec4 color;
    float intensity;
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
    float radius;
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

uniform AmbientLight ambientLight;

uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;

uniform PointLight pointLights[32];
uniform int pointLightCount;

uniform SpotLight spotlights[32];
uniform int spotlightCount;

uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;

uniform vec3 cameraPosition;

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    return vec4(0.0);
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
    return vec2(0);
}

float calculateShadow(ShadowParameters shadowParam, vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = shadowParam.lightProjection * shadowParam.lightView * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDir = normalize(-directionalLights[0].direction);
    float biasValue = shadowParam.bias;
    float bias = max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - fragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));

    int sampleCount = 0;
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = sampleTextureAt(shadowParam.textureIndex,
                projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);

    return shadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos) {
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 20;
    const vec3 sampleOffsetDirections[] = vec3[](
        vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
        vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
        vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
        vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
        vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
        vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
        vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
        vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
        vec3(-0.1120, 0.1234, -0.7446), vec3(-0.2130, -0.0782, -0.1379),
        vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843)
    );

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);
    return shadow;
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 diffuseMat, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(-light.direction);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * diffuseMat;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * specColor * light.specular;
    
    return diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 diffuseMat, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * diffuseMat;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * specColor * light.specular;
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    
    return (diffuse + specular) * attenuation;
}

vec3 calcSpotLight(SpotLight light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 diffuseMat, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * diffuseMat;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * specColor * light.specular;
    
    vec3 spotDirection = normalize(light.direction);
    float theta = dot(lightDir, -spotDirection);
    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    return (diffuse + specular) * intensity * attenuation;
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    return color;
}

void main() {
    vec4 posDepth = texture(gPosition, TexCoord);
    vec3 FragPos = posDepth.xyz;
    
    vec3 Normal = texture(gNormal, TexCoord).rgb;
    
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoord);
    vec3 Albedo = albedoSpec.rgb;
    float SpecIntensity = albedoSpec.a;

    vec4 matData = texture(gMaterial, TexCoord);
    float shininess = matData.a * 256.0;
    float reflectivity = matData.b;
    
    vec3 viewDir = normalize(cameraPosition - FragPos);
    vec3 specColor = vec3(SpecIntensity);
    
    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * Albedo;
    
    float dirShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (!shadowParams[i].isPointLight) {
            dirShadow = max(dirShadow, calculateShadow(shadowParams[i], FragPos, Normal));
        }
    }
    
    float pointShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (shadowParams[i].isPointLight) {
            pointShadow = max(pointShadow, calculatePointShadow(shadowParams[i], FragPos));
        }
    }
    
    vec3 directionalResult = vec3(0.0);
    for (int i = 0; i < directionalLightCount; i++) {
        directionalResult += calcDirectionalLight(directionalLights[i], Normal, viewDir, 
            Albedo, specColor, shininess);  
    }
    directionalResult *= (1.0 - dirShadow);

    vec3 pointResult = vec3(0.0);
    for (int i = 0; i < pointLightCount; i++) {
        float distance = length(pointLights[i].position - FragPos);
        if (distance > pointLights[i].radius) continue;
        pointResult += calcPointLight(pointLights[i], FragPos, Normal, viewDir,
            Albedo, specColor, shininess);  
    }
    pointResult *= (1.0 - pointShadow);

    vec3 spotResult = vec3(0.0);
    for (int i = 0; i < spotlightCount; i++) {
        spotResult += calcSpotLight(spotlights[i], FragPos, Normal, viewDir,
            Albedo, specColor, shininess);  
    }

    vec3 finalColor = ambient + directionalResult + pointResult + spotResult;
    
    if (reflectivity > 0.0) {
        vec3 I = normalize(FragPos - cameraPosition);
        vec3 R = reflect(I, normalize(Normal));
        vec3 envColor = texture(skybox, R).rgb;
        finalColor = mix(finalColor, envColor, reflectivity);
    }
    
    FragColor = vec4(finalColor, 1.0);
    
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    
    FragColor.rgb = acesToneMapping(FragColor.rgb);
}