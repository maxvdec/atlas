#version 450
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    bool isInstanced;
};

invariant gl_Position;

void main() {
    if (isInstanced) {
        gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
}
