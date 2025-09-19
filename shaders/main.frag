#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;

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

void main() {
    if (useTexture && !useColor) {
        FragColor = calculateAllTextures(); 
    }

    if (useTexture && useColor) {
        FragColor = calculateAllTextures() * outColor;
    } else if (!useTexture && useColor) {
        FragColor = outColor;
    }

    vec4 ambient = vec4(ambientLight.color.rgb * ambientLight.intensity, 1.0);
    FragColor = FragColor * ambient;
}
