#version 330 core

layout(location = 0) in vec3 quadVertex;
layout(location = 1) in vec2 texCoord;

layout(location = 2) in vec3 particlePos;
layout(location = 3) in vec4 particleColor;
layout(location = 4) in float particleSize;

uniform mat4 view;
uniform mat4 projection;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPosition = particlePos +
                         (quadVertex.x * cameraRight * particleSize) +
                         (quadVertex.y * cameraUp * particleSize);

    vec4 viewPos = view * vec4(worldPosition, 1.0);
    gl_Position = projection * viewPos;

    fragTexCoord = texCoord;
    fragColor = particleColor;
}