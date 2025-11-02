#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 WorldNormal;
out vec3 WorldTangent;
out vec3 WorldBitangent;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    WorldNormal = normalize(mat3(model) * aNormal);
    WorldTangent = normalize(mat3(model) * aTangent);
    WorldBitangent = normalize(mat3(model) * aBitangent);
}
