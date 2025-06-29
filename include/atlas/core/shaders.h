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

static const char* AMBIENT_FRAG = R"(
#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D specularMap;
    bool useSpecularMap;
};

uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uCameraPos;

out vec4 FragColor;

void main() {
    // Get base color
    vec4 baseColor;
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, texCoord);
        if (texColor.a < 0.1) {
            baseColor = texColor;
        } else {
            baseColor = texColor * fragColor;
        }
    } else {
        baseColor = fragColor;
    }
    
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLight.position - fragPos);
    vec3 viewDir = normalize(uCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    vec3 ambient = uLight.ambient * uLight.color * uLight.intensity;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight.diffuse * uLight.color * diff * uLight.intensity;
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    
    vec3 specularColor = uMaterial.specular;
    if (uMaterial.useSpecularMap) {
        specularColor = texture(uMaterial.specularMap, texCoord).rgb;
    }
    vec3 specular = uLight.specular * uLight.color * spec * specularColor * uLight.intensity;
    
    vec3 ambientDiffuse = (ambient + diffuse) * uMaterial.diffuse;
    vec3 finalColor = baseColor.rgb * ambientDiffuse + specular;
    
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

#endif // ATLAS_GENERATED_SHADERS_H
