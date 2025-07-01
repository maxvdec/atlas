// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* NORMAL_FRAG = R"(
#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;

uniform bool uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        if (fragColor.a < 0.01) {
            FragColor = texture(uTexture, texCoord);
            FragColor = texture(uTexture, texCoord) * vec4(fragColor.xyz, 1.0);
        }
    } else {
        FragColor = vec4(fragColor);
    }
}

)";

static const char* FULLSCREEN_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 textCoord;

uniform sampler2D uTexture1;
uniform bool uInverted;
uniform bool uGrayscale;
uniform bool uKernel;
uniform float uKernelStrength;
uniform vec2 uTexelSize;

void main() {
    vec3 color = texture(uTexture1, textCoord).rgb;
    
    if (uInverted) {
        color = vec3(1.0) - color; 
    }

    if (uGrayscale) {
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(gray);
    }

    vec2 offsets[9] = vec2[](
        vec2(-1,  1), vec2(0,  1), vec2(1,  1),
        vec2(-1,  0), vec2(0,  0), vec2(1,  0),
        vec2(-1, -1), vec2(0, -1), vec2(1, -1)
    );

    if (uKernel) {
        float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );

        vec3 sampleTex[9];
        for (int i = 0; i < 9; i++) {
            vec2 samplePos = textCoord + offsets[i] * uTexelSize;
            sampleTex[i] = texture(uTexture1, samplePos).rgb;
        }

        vec3 result = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            result += sampleTex[i] * kernel[i];
        }

        result = mix(color, result, uKernelStrength);
        color = result;
    }

    FragColor = vec4(color, 1.0);
}

)";

static const char* AMBIENT_FRAG = R"(
#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

struct DirectionalLight {
    vec3 direction;
};

struct PointLight {
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 direction;
    float cutOff;
    float outerCutOff;  
};

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool isDirectional;
    DirectionalLight directionalLight;
    bool isPointLight;
    PointLight pointLight;
    bool isSpotLight;
    SpotLight spotLight;
};

struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D specularMap1;
    sampler2D specularMap2;
    int specularMapCount;
    bool useSpecularMap;
};

uniform bool uUseTexture;
uniform sampler2D uTexture1;
uniform sampler2D uTexture2;
uniform sampler2D uTexture3;
uniform int uTextureCount;
uniform Material uMaterial;
uniform vec3 uCameraPos;

#define MAX_LIGHTS 10
uniform int uLightCount;
uniform Light uLights[MAX_LIGHTS];

out vec4 FragColor;

vec4 blendDiffuseTextures() {
    vec4 finalTexture = vec4(1.0);
    
    if (uTextureCount >= 1) {
        finalTexture = texture(uTexture1, texCoord);
    }
    
    if (uTextureCount >= 2) {
        vec4 tex2 = texture(uTexture2, texCoord);
        finalTexture = mix(finalTexture, tex2, 0.5); 
    }
    
    if (uTextureCount >= 3) {
        vec4 tex3 = texture(uTexture3, texCoord);
        finalTexture = mix(finalTexture, tex3, 0.33); 
    }
    
    return finalTexture;
}

vec3 blendSpecularTextures() {
    vec3 finalSpecular = vec3(1.0);
    
    if (uMaterial.specularMapCount >= 1) {
        finalSpecular = texture(uMaterial.specularMap1, texCoord).rgb;
    }
    
    if (uMaterial.specularMapCount >= 2) {
        vec3 spec2 = texture(uMaterial.specularMap2, texCoord).rgb;
        finalSpecular = (finalSpecular + spec2) * 0.5;
    }
    
    return finalSpecular;
}

vec3 calculateLighting(Light light, vec3 norm, vec3 viewDir, vec3 fragPos, vec3 materialDiffuse) {
    vec3 lightDir;
    if (light.isDirectional) {
        lightDir = normalize(-light.directionalLight.direction);
    } else {
        lightDir = normalize(light.position - fragPos);
    }
    
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float attenuation = 1.0;
    float spotlightEffect = 1.0;
    
    if (light.isPointLight) {
        float dist = length(light.position - fragPos);
        attenuation = 1.0 / (light.pointLight.constant + 
                           light.pointLight.linear * dist + 
                           light.pointLight.quadratic * (dist * dist));
    }

    if (light.isSpotLight) {
        vec3 lightToFrag = normalize(fragPos - light.position);
        vec3 spotDirection = normalize(light.spotLight.direction);
        float theta = dot(lightToFrag, spotDirection);
        
        if (theta > light.spotLight.outerCutOff) {
            float epsilon = light.spotLight.cutOff - light.spotLight.outerCutOff;
            if (epsilon > 0.0) {
                float intensity = clamp((theta - light.spotLight.outerCutOff) / epsilon, 0.0, 1.0);
                spotlightEffect = intensity;
            } else {
                spotlightEffect = 1.0;
            }
        } else {
            spotlightEffect = 0.0;
        }
    }
    
    vec3 ambient = light.ambient * light.color * materialDiffuse;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * light.color * diff * materialDiffuse * attenuation * spotlightEffect;
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specularColor = uMaterial.specular;
    if (uMaterial.useSpecularMap) {
        specularColor = blendSpecularTextures();
    }
    vec3 specular = light.specular * light.color * spec * specularColor * attenuation * spotlightEffect;
    
    return (ambient + diffuse + specular) * light.intensity;
}

void main() {
    vec4 baseColor;
    vec3 materialDiffuse;

    if (uUseTexture) {
        vec4 texColor = blendDiffuseTextures();
        if (texColor.a < 0.1) {
            discard; 
        }
        baseColor = texColor * fragColor;
        materialDiffuse = texColor.rgb ;
    } else {
        baseColor = fragColor;
        materialDiffuse = uMaterial.diffuse;
    }
    
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(uCameraPos - fragPos);
    
    vec3 totalLighting = vec3(0.0);
    
    for (int i = 0; i < min(uLightCount, MAX_LIGHTS); ++i) {
        totalLighting += calculateLighting(uLights[i], norm, viewDir, fragPos, materialDiffuse);
    }
    
    vec3 finalColor = totalLighting;
    
    FragColor = vec4(finalColor, baseColor.a);
}

)";

static const char* MAIN_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec4 fragColor;
out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 uAspectCorrection;

void main()
{
    fragColor = aColor;
    texCoord = aTexCoord;
    normal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    fragPos = vec3(uModel * vec4(aPos, 1.0));
}

)";

static const char* FULLSCREEN_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec2 textCoord;

void main() {
    textCoord = aTexCoord;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
