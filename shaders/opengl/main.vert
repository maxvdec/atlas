#version 410 core
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
    mat4 modelMatrix = model;
    if (isInstanced) {
        modelMatrix = instanceModel;
    }

    mat4 mvp = projection * view * modelMatrix;
    gl_Position = mvp * vec4(aPos, 1.0);

    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    outColor = aColor;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    vec3 N = Normal;
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    TBN = mat3(T, B, N);
}
