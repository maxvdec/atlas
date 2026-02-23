#version 450
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec2 TexCoord;

void main() {
    // Pass through UVs; G-buffer was rendered with the same orientation
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}
