#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 fragColor;

void main()
{
    fragColor = aColor;
    gl_Position = vec4(aPos, 1.0);
}
