#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

struct Material {
    vec3 ambient;
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

    vec3 ambient = uMaterial.ambient * uLight.color * uLight.intensity;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLight.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight.color * (diff * uMaterial.diffuse) * uLight.intensity;

    vec3 viewDir = normalize(uCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess) * uLight.intensity;
    vec3 specular = uLight.color * (spec * uMaterial.specular);

    vec3 result = uMaterial.ambient + diffuse + specular;
    FragColor = vec4(result, FragColor.a);
}
