#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;

uniform bool uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        if (fragColor.a < 0.01) {
            FragColor = texture(uTexture, texCoord);
            FragColor = texture(uTexture, texCoord) * vec4(fragColor.xyz, 1.0);
        }
    } else {
        FragColor = vec4(fragColor);
    }
}
