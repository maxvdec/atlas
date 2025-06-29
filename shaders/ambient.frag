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
