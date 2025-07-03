#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec4 fragColor;
out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;
out vec4 lightSpacePos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 uAspectCorrection;
uniform mat4 uLightSpaceMatrix;
uniform mat3 uNormalMatrix;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    fragColor = aColor;
    texCoord = aTexCoord;
    normal = uNormalMatrix * aNormal;
    lightSpacePos = uLightSpaceMatrix * worldPos;
    fragPos = vec3(worldPos); 
    gl_Position = uProjection * uView * worldPos;
}
