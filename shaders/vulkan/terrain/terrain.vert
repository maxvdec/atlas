#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out vec2 TexCoord;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}