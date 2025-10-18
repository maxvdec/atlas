#version 410 core
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
