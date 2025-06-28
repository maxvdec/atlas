#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 fragColor;

void main()
{
    fragPos = aPos;
    gl_Position = vec4(aPos, 1.0);
}
