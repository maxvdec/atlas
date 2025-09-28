#version 330 core

in vec4 passColor;

out vec4 FragColor;

uniform sampler2D particleText;
uniform bool useTexture;

void main() {
    if (!useTexture) {
        FragColor = passColor;
        return;
    }
    vec2 uv = gl_PointCoord;
    vec4 tex = texture(particleText, uv);
    if (tex.a < 0.1) discard;
    FragColor = tex * passColor;
}