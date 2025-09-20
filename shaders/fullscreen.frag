#version 330 core

uniform sampler2D screenTexture;

in vec2 TexCoord;

out vec4 FragColor;

void main() {
    vec3 col = texture(screenTexture, TexCoord).rgb;
    FragColor = vec4(col, 1.0);
}
