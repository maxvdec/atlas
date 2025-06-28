#version 330 core
in vec3 fragColor;
in vec2 texCoord;

uniform bool uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        FragColor = texture(uTexture, texCoord);
    } else {
        FragColor = vec4(fragColor, 1.0);
    }
}
