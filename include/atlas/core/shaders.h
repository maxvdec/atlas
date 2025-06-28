// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* NORMAL_FRAG = R"(
#version 330 core
in vec3 fragColor;
in vec2 texCoord;

uniform bool uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        FragColor = texture(uTexture, texCoord);
    } else {
        FragColor = vec4(fragColor, 1.0);
    }
}

)";

static const char* NORMAL_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 fragColor;
out vec2 texCoord;

void main()
{
    fragColor = aColor;
    texCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
