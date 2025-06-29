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
};

uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uCameraPos;

out vec4 FragColor;
void main() {
    if (uUseTexture) {
        if (fragColor.a < 0.01) {
            FragColor = texture(uTexture, texCoord);
        } else {
            FragColor = texture(uTexture, texCoord) * vec4(fragColor.xyz, 1.0);
        }
    } else {
        FragColor = vec4(fragColor);
    }

    vec3 ambient = uLight.color * uLight.intensity * uLight.ambient;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLight.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight.color * (diff * uMaterial.diffuse) * uLight.intensity;
    diffuse *= uLight.diffuse;

    vec3 viewDir = normalize(uCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess) * uLight.intensity;
    vec3 specular = uLight.color * (spec * uMaterial.specular);
    specular *= uLight.specular;

    vec3 result = diffuse + specular;
    FragColor = vec4(result, FragColor.a);
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
