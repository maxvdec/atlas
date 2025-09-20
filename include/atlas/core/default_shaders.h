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

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;

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

struct Light {
    vec3 position;

    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D textures[16];
uniform int textureTypes[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;
uniform Light light;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;

vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0; 
    
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) { 
            color += texture(textures[i], TexCoord);
            count++;
        }
    }
    
    if (count > 0) {
        color /= float(count); 
    }

    if (count == 0) {
        return vec4(-1.0);
    }
    
    return color;
}

vec3 calculateDiffuse() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * material.diffuse) * light.diffuse;
    return diffuse;
}

vec3 calculateSpecular() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPosition - FragPos);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    vec4 specularTexture = enableTextures(TEXTURE_SPECULAR);
    vec3 specularColor = material.specular;
    
    if (specularTexture.r != -1.0 || specularTexture.g != -1.0 || specularTexture.b != -1.0) {
        specularColor *= specularTexture.rgb;
    }
    
    vec3 specular = (spec * specularColor) * light.specular;
    return specular;
}


void main() {
    vec4 baseColor;
    if (useTexture && !useColor) {
        baseColor = enableTextures(TEXTURE_COLOR); 
    } else if (useTexture && useColor) {
        baseColor = enableTextures(TEXTURE_COLOR) * outColor;
    } else if (!useTexture && useColor) {
        baseColor = outColor;
    } else {
        baseColor = vec4(1.0); 
    }
    
   
    vec3 ambient = (ambientLight.color.rgb * ambientLight.intensity) * material.ambient; 

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
