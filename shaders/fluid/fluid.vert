#version 410 core

layout(location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);

    gl_PointSize = 10.0;
    FragPos = aPos;
}