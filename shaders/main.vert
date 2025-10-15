#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

out vec4 outColor;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

void main() {
    mat4 mvp;
    if (isInstanced) {
        mvp = projection * view * instanceModel;
    } else {
        mvp = projection * view * model;
    }
    gl_Position = mvp * vec4(aPos, 1.0);
    FragPos = vec3(instanceModel * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
    outColor = aColor;

    vec3 T = normalize(vec3(instanceModel * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(instanceModel * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(instanceModel * vec4(aNormal, 0.0)));
    TBN = mat3(T, B, N);
}
