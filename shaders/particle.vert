#version 330 core

// Quad vertex attributes
layout(location = 0) in vec3 quadVertex; // Local quad position (-0.5 to 0.5)
layout(location = 1) in vec2 texCoord;   // Texture coordinates

// Instance attributes (per particle)
layout(location = 2) in vec3 particlePos;   // World position
layout(location = 3) in vec4 particleColor; // Color and alpha
layout(location = 4) in float particleSize; // Size in world units

uniform mat4 view;
uniform mat4 projection;
uniform bool isAmbient;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    if (isAmbient) {
        vec4 viewParticlePos = view * vec4(particlePos, 1.0);

        vec3 viewPosition =
            viewParticlePos.xyz +
            vec3(quadVertex.x * particleSize, quadVertex.y * particleSize, 0.0);

        gl_Position = projection * vec4(viewPosition, 1.0);
    } else {
        vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);

        vec3 worldPosition = particlePos +
                             (quadVertex.x * cameraRight * particleSize) +
                             (quadVertex.y * cameraUp * particleSize);

        gl_Position = projection * view * vec4(worldPosition, 1.0);
    }

    fragTexCoord = texCoord;
    fragColor = particleColor;
}