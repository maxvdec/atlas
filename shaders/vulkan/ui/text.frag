#version 450

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 color;

layout(set = 2, binding = 0) uniform sampler2D text;
layout(set = 1, binding = 0) uniform TextColor {
    vec3 textColor;
};

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, texCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}