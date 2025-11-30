#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform bool isInstanced = true;

void main() {
    if (isInstanced) {
        gl_Position = model * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = model * vec4(aPos, 1.0);
    }
}