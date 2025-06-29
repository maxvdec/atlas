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
uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uCameraPos;

out vec4 FragColor;

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
    
    vec3 lightDir;
    if (uLight.isDirectional) {
        lightDir = normalize(-uLight.directionalLight.direction);
    } else {
        lightDir = normalize(uLight.position - fragPos);
    }
    
    vec3 viewDir = normalize(uCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float attenuation = 1.0;
    float spotlightEffect = 1.0;
    
    // Point light attenuation
    if (uLight.isPointLight) {
        float dist = length(uLight.position - fragPos);
        attenuation = 1.0 / (uLight.pointLight.constant + 
                           uLight.pointLight.linear * dist + 
                           uLight.pointLight.quadratic * (dist * dist));
    }
    
    // Spotlight calculation
    if (uLight.isSpotLight) {
        vec3 lightToFrag = normalize(fragPos - uLight.position);
        vec3 spotDirection = normalize(uLight.spotLight.direction);
        float theta = dot(lightToFrag, spotDirection);
        
        // Check if fragment is within the spotlight cone
        if (theta > uLight.spotLight.outerCutOff) {
            float epsilon = uLight.spotLight.cutOff - uLight.spotLight.outerCutOff;
            float intensity = clamp((theta - uLight.spotLight.outerCutOff) / epsilon, 0.0, 1.0);
            spotlightEffect = intensity;
        } else {
            spotlightEffect = 0.0; // Outside the cone
        }
        
        // Apply distance attenuation for spotlight as well
        if (uLight.isPointLight) {
            // Use point light attenuation settings for spotlight distance falloff
            float dist = length(uLight.position - fragPos);
            attenuation = 1.0 / (uLight.pointLight.constant + 
                               uLight.pointLight.linear * dist + 
                               uLight.pointLight.quadratic * (dist * dist));
        }
    }
    
    vec3 ambient = uLight.ambient * uLight.color * uMaterial.diffuse;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight.diffuse * uLight.color * diff * uMaterial.diffuse * attenuation * spotlightEffect;
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specularColor = uMaterial.specular;
    if (uMaterial.useSpecularMap) {
        specularColor = texture(uMaterial.specularMap, texCoord).rgb;
    }
    vec3 specular = uLight.specular * uLight.color * spec * specularColor * attenuation * spotlightEffect;
    
    vec3 lighting = (ambient + diffuse + specular) * uLight.intensity;
    
    vec3 finalColor = baseColor.rgb * lighting;
    
    FragColor = vec4(finalColor, baseColor.a);
}
