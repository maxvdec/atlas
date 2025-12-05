#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in mat4 instanceModel;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    bool isInstanced;
}
uniforms;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec3 FragPos;
layout(location = 4) out mat3 TBN;

void main() {
    mat4 modelMatrix = uniforms.model;
    if (uniforms.isInstanced) {
        modelMatrix = instanceModel;
    }

    mat4 mvp = uniforms.projection * uniforms.view * modelMatrix;
    gl_Position = mvp * vec4(aPos, 1.0);

    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    // Flip V coordinate to match texture orientation (textures are flipped on
    // load)
    TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
    outColor = aColor;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    vec3 N = Normal;
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    TBN = mat3(T, B, N);
}
