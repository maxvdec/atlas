#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 6) in mat4 instanceModel;

layout(location = 0) out vec4 vertexColor;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    bool isInstanced;
};

void main() {
    mat4 mvp;
    if (isInstanced) {
        mvp = projection * view * instanceModel;
    } else {
        mvp = projection * view * model;
    }
    gl_Position = mvp * vec4(aPos, 1.0);
    vertexColor = aColor;
}
