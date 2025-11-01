#version 410 core
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 brightColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec4 outColor;
in mat3 TBN;

void main() {
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
