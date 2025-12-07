#version 450
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    bool isInstanced;
};

layout(set = 2, binding = 0) uniform ShadowUniforms {
    mat4 shadowMatrix; // Single shadow matrix for current face
    int faceIndex;     // Which cubemap face we're rendering (0-5)
};

layout(location = 0) out vec4 FragPos;

void main() {
    vec4 worldPos;
    if (isInstanced) {
        worldPos = model * instanceModel * vec4(aPos, 1.0);
    } else {
        worldPos = model * vec4(aPos, 1.0);
    }

    FragPos = worldPos;
    gl_Position = shadowMatrix * worldPos;
}
