// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* COLOR_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec4 vertexColor;

void main() {
    FragColor = vertexColor;
}

)";

static const char* MAIN_FRAG = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;

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

uniform sampler2D textures[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;

vec3 lightPos = vec3(3.0, 1.0, 0.0); // Example light position

vec4 calculateAllTextures() {
    vec4 color = vec4(0.0);
    for (int i = 0; i <= textureCount; i++) {
        color += texture(textures[i], TexCoord);
    }
    color /= float(textureCount + 1); 
    return color;
}

vec3 calculateDiffuse() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * material.diffuse) * vec3(1.0); // Assuming white light for simplicity 
    return diffuse;
}

vec3 calculateSpecular() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPosition - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm); // Use normalized normal
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess); 
    vec3 specular = (spec * material.specular) * vec3(1.0);
    return specular;
}

void main() {
    vec4 baseColor;
    if (useTexture && !useColor) {
        baseColor = calculateAllTextures(); 
    } else if (useTexture && useColor) {
        baseColor = calculateAllTextures() * outColor;
    } else if (!useTexture && useColor) {
        baseColor = outColor;
    } else {
        baseColor = vec4(1.0); 
    }
    
    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * material.ambient;
    vec3 diffuse = calculateDiffuse();
    vec3 specular = calculateSpecular();
    
    vec3 finalColor = baseColor.rgb * (ambient + diffuse) + specular;
    FragColor = vec4(finalColor, baseColor.a);
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

static const char* COLOR_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
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

#endif // ATLAS_GENERATED_SHADERS_H
