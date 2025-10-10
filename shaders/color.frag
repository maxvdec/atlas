#version 330 core
out vec4 FragColor;
in vec4 vertexColor;

void main() {
    vec3 color = vertexColor.rgb / (vertexColor.rgb + vec3(1.0));
    FragColor = vec4(color, vertexColor.a);
}