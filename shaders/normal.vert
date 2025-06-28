#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 fragColor;
out vec2 texCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 uAspectCorrection;

void main()
{
    fragColor = aColor;
    texCoord = aTexCoord;
    gl_Position = uModel * uView * uProjection * vec4(aPos, 1.0);
    gl_Position.x *= uAspectCorrection.x;
    gl_Position.y *= uAspectCorrection.y;
}
