// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* NORMAL_FRAG = R"(
#version 330 core
in vec3 fragPos;

out vec4 FragColor;

void main() {
    FragColor = vec4(fragPos, 1.0); // Output the fragment position as color
}

)";

static const char* NORMAL_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 fragColor;

void main()
{
    fragPos = aPos;
    gl_Position = vec4(aPos, 1.0);
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
