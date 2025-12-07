#version 450
layout(location = 0) in vec4 vertex; // <vec2 pos, vec2 texture>

layout(location = 0) out vec2 texCoords;

layout(set = 0, binding = 0) uniform Uniforms { mat4 projection; };

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    texCoords = vertex.zw;
}