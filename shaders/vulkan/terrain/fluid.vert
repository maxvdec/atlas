#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 WorldPos;
layout(location = 2) out vec3 WorldNormal;
layout(location = 3) out vec3 WorldTangent;
layout(location = 4) out vec3 WorldBitangent;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    WorldNormal = normalize(mat3(model) * aNormal);
    WorldTangent = normalize(mat3(model) * aTangent);
    WorldBitangent = normalize(mat3(model) * aBitangent);
}