#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 6) in mat4 instanceModel;

out vec4 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

void main() {
    mat4 mvp;
    if (isInstanced) {
        mvp = projection * view * instanceModel;
    } else {
        mvp = projection * view * model;
    }
    gl_Position = mvp * vec4(aPos, 1.0);
    vertexColor = aColor;
}
