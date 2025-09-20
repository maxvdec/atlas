#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textures[16];

void main() {
    vec3 col = texture(textures[0], TexCoord).rgb;
    FragColor = vec4(col, 1.0);
}
