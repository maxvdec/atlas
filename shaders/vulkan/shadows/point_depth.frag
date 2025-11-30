#version 450
layout(location = 0) in vec4 FragPos;

layout(set = 1, binding = 0) uniform Uniforms {
    vec3 lightPos;
    float far_plane;
};

void main() {
    float lightDistance = length(FragPos.xyz - lightPos);
    
    lightDistance = lightDistance / far_plane;

    gl_FragDepth = lightDistance;
}