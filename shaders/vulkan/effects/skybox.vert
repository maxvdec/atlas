#version 450
layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 TexCoords;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projection;
    mat4 view;
};

void main() {
    TexCoords = aPos;
    // Drop translation so the cube stays centered on the camera
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
