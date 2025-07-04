#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec2 textCoord;

void main() {
    textCoord = aTexCoord;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
