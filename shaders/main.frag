#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;

uniform sampler2D texture1;
uniform bool useTexture;
uniform bool onlyTexture;

void main() {
    if (onlyTexture) {
        FragColor = texture(texture1, TexCoord);
        return;
    }

    if (useTexture) {
        FragColor = texture(texture1, TexCoord) * outColor;
    } else {
        FragColor = outColor;
    }
}
