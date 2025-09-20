#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D Texture;

void main() {
    vec3 col = texture(Texture, TexCoord).rgb;
    FragColor = vec4(col, 1.0);
}
