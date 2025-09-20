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
