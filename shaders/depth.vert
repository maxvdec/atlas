#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection; // the light space matrix
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);
}
