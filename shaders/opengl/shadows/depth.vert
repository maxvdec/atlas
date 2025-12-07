#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

uniform mat4 projection; // the light space matrix
uniform mat4 view;
uniform mat4 model;
uniform bool isInstanced = true;

void main() {
    if (isInstanced) {
        gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
}
