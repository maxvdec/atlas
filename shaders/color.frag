#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
in vec4 vertexColor;

void main() {
    vec3 color = vertexColor.rgb / (vertexColor.rgb + vec3(1.0));
    FragColor = vec4(color, vertexColor.a);
    BrightColor = vec4(color * 2.0, vertexColor.a);
}