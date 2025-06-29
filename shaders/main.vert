#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec4 fragColor;
out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 uAspectCorrection;

void main()
{
    fragColor = aColor;
    texCoord = aTexCoord;
    normal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    fragPos = vec3(uModel * vec4(aPos, 1.0));
}
