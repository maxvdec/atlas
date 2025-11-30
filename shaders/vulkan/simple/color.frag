#version 450
layout(location = 0) in vec4 vertexColor;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

void main() {
    vec3 color = vertexColor.rgb / (vertexColor.rgb + vec3(1.0));
    FragColor = vec4(color, vertexColor.a);
    if (length(color) > 1.0) {
        BrightColor = vec4(color, vertexColor.a);
    }
}