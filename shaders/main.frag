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

uniform sampler2D textures[16];

uniform AmbientLight ambientLight;

uniform bool useTexture;
uniform bool useColor;
uniform int textureCount;

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
    vec3 lightPos = vec3(3.0, 1.0, 0.0); // Example light position
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0); // Assuming white light for simplicity 

    return diffuse;
}

void main() {
    if (useTexture && !useColor) {
        FragColor = calculateAllTextures(); 
    }

    if (useTexture && useColor) {
        FragColor = calculateAllTextures() * outColor;
    } else if (!useTexture && useColor) {
        FragColor = outColor;
    }

    vec3 ambient = vec3(ambientLight.color.rgb * ambientLight.intensity);
    vec3 diffuse = calculateDiffuse();

    FragColor =  vec4(ambient + diffuse, 1.0) * FragColor;
}
