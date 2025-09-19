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

struct Light {
    vec3 position;

    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D textures[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;
uniform Light light;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;

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
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * material.diffuse) * light.diffuse;
    return diffuse;
}

vec3 calculateSpecular() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPosition - FragPos);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm); // Use normalized normal
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess); 
    vec3 specular = (spec * material.specular) * light.specular;
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
    
   
    vec3 ambient = (ambientLight.color.rgb * ambientLight.intensity) * material.ambient; 

    vec3 diffuse = calculateDiffuse();
    vec3 specular = calculateSpecular();
    
    vec3 finalColor = baseColor.rgb * (ambient + diffuse) + specular;
    FragColor = vec4(finalColor, baseColor.a);
}
