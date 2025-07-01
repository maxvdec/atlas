#version 330 core

in vec3 texCoord;
out vec4 FragColor;

uniform samplerCube uSkyboxTexture;

void main() {
    FragColor = texture(uSkyboxTexture, texCoord);
}
