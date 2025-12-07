#version 450
layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec4 outColor;

layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D texture1;
layout(set = 2, binding = 1) uniform sampler2D texture2;
layout(set = 2, binding = 2) uniform sampler2D texture3;
layout(set = 2, binding = 3) uniform sampler2D texture4;
layout(set = 2, binding = 4) uniform sampler2D texture5;
layout(set = 2, binding = 5) uniform sampler2D texture6;
layout(set = 2, binding = 6) uniform sampler2D texture7;
layout(set = 2, binding = 7) uniform sampler2D texture8;
layout(set = 2, binding = 8) uniform sampler2D texture9;
layout(set = 2, binding = 9) uniform sampler2D texture10;
layout(set = 2, binding = 10) uniform sampler2D texture11;
layout(set = 2, binding = 11) uniform sampler2D texture12;
layout(set = 2, binding = 12) uniform sampler2D texture13;
layout(set = 2, binding = 13) uniform sampler2D texture14;
layout(set = 2, binding = 14) uniform sampler2D texture15;
layout(set = 2, binding = 15) uniform sampler2D texture16;

layout(set = 1, binding = 0) uniform UBO {
    bool useTexture;
    bool onlyTexture;
    int textureCount;
};

vec4 calculateAllTextures() {
    vec4 color = vec4(0.0);

    for (int i = 0; i < textureCount; i++) {
        if (i == 0) {
            color += texture(texture1, TexCoord);
        } else if (i == 1) {
            color += texture(texture2, TexCoord);
        } else if (i == 2) {
            color += texture(texture3, TexCoord);
        } else if (i == 3) {
            color += texture(texture4, TexCoord);
        } else if (i == 4) {
            color += texture(texture5, TexCoord);
        } else if (i == 5) {
            color += texture(texture6, TexCoord);
        } else if (i == 6) {
            color += texture(texture7, TexCoord);
        } else if (i == 7) {
            color += texture(texture8, TexCoord);
        } else if (i == 8) {
            color += texture(texture9, TexCoord);
        } else if (i == 9) {
            color += texture(texture10, TexCoord);
        } else if (i == 10) {
            color += texture(texture11, TexCoord);
        } else if (i == 11) {
            color += texture(texture12, TexCoord);
        } else if (i == 12) {
            color += texture(texture13, TexCoord);
        } else if (i == 13) {
            color += texture(texture14, TexCoord);
        } else if (i == 14) {
            color += texture(texture15, TexCoord);
        } else if (i == 15) {
            color += texture(texture16, TexCoord);
        }
    }

    color /= float(textureCount); 

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