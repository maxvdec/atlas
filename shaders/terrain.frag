#version 330 core

in vec3 FragPos;
in float Height;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main() {
    float h = (Height + 16.0) / 32.0;
    FragColor = vec4(h, h, h, 1.0);
}