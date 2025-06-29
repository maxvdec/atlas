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
    sampler2D specularMap;
    bool useSpecularMap;
};

uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform Material uMaterial;
uniform vec3 uCameraPos;

#define MAX_LIGHTS 10
uniform int uLightCount;
uniform Light uLights[MAX_LIGHTS];

out vec4 FragColor;

vec3 calculateLighting(Light light, vec3 norm, vec3 viewDir, vec3 fragPos) {
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
    
    vec3 ambient = light.ambient * light.color * uMaterial.diffuse;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * light.color * diff * uMaterial.diffuse * attenuation * spotlightEffect;
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specularColor = uMaterial.specular;
    if (uMaterial.useSpecularMap) {
        specularColor = texture(uMaterial.specularMap, texCoord).rgb;
    }
    vec3 specular = light.specular * light.color * spec * specularColor * attenuation * spotlightEffect;
    
    return (ambient + diffuse + specular) * light.intensity;
}

void main() {
    vec4 baseColor;
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, texCoord);
        if (texColor.a < 0.1) {
            discard; 
        }
        baseColor = texColor * fragColor;
    } else {
        baseColor = fragColor;
    }
    
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(uCameraPos - fragPos);
    
    vec3 totalLighting = vec3(0.0);
    
    for (int i = 0; i < min(uLightCount, MAX_LIGHTS); ++i) {
        totalLighting += calculateLighting(uLights[i], norm, viewDir, fragPos);
    }
    
    vec3 finalColor = baseColor.rgb * totalLighting;
    
    FragColor = vec4(finalColor, baseColor.a);
}
