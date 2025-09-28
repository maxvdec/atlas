#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

out vec4 passColor;

uniform mat4 viewProj;

void main() {
    gl_Position = viewProj * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    passColor = inColor;
}