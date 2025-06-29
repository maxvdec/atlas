// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* NORMAL_FRAG = R"(
#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;

uniform bool uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        if (fragColor.a < 0.01) {
            FragColor = texture(uTexture, texCoord);
            FragColor = texture(uTexture, texCoord) * vec4(fragColor.xyz, 1.0);
        }
    } else {
        FragColor = vec4(fragColor);
    }
}

)";

static const char* AMBIENT_FRAG = R"(
#version 330 core
in vec4 fragColor;
in vec2 texCoord;
in vec3 normal;

uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform vec3 uAmbientColor;
uniform vec3 uLightPos;

out vec4 FragColor;

void main() {
    FragColor = vec4(normal, 1.0);
    return;
    if (uUseTexture) {
        if (fragColor.a < 0.01) {
            FragColor = texture(uTexture, texCoord);
        } else {
            FragColor = texture(uTexture, texCoord) * vec4(fragColor.xyz, 1.0);
        }
    } else {
        FragColor = vec4(fragColor);
    }

    FragColor.rgb *= uAmbientColor;
}

)";

static const char* MAIN_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec4 fragColor;
out vec2 texCoord;
out vec3 normal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 uAspectCorrection;

void main()
{
    fragColor = aColor;
    texCoord = aTexCoord;
    normal = aNormal;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
