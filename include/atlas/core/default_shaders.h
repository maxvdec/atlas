// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* COLOR_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec4 vertexColor;

void main() {
    FragColor = vertexColor;
}

)";

static const char* MAIN_FRAG = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;

uniform sampler2D texture1;
uniform bool useTexture;
uniform bool onlyTexture;

void main() {
    if (onlyTexture) {
        FragColor = texture(texture1, TexCoord);
        return;
    }

    if (useTexture) {
        FragColor = texture(texture1, TexCoord) * outColor;
    } else {
        FragColor = outColor;
    }
}

)";

static const char* DEBUG_FRAG = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}

)";

static const char* COLOR_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}

)";

static const char* DEBUG_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

)";

static const char* MAIN_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 outColor;
out vec2 TexCoord; 

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    outColor = aColor;
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
