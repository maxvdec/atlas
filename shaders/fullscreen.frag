#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

const int TEXTURE_COLOR = 0;
const int TEXTURE_DEPTH = 3;

uniform sampler2D Texture;
uniform int TextureType;

void main() {
    if (TextureType == TEXTURE_COLOR) {
        vec3 col = texture(Texture, TexCoord).rgb;
        FragColor = vec4(col, 1.0);
    } else if (TextureType == TEXTURE_DEPTH) {
        float depth = texture(Texture, TexCoord).r;
        FragColor = vec4(vec3(depth), 1.0);
    }
}
