#version 330 core
out vec4 FragColor;

in vec2 textCoord;

uniform sampler2D uTexture1;
uniform bool uInverted;
uniform bool uGrayscale;
uniform bool uKernel;
uniform float uKernelStrength;
uniform vec2 uTexelSize;

void main() {
    vec3 color = texture(uTexture1, textCoord).rgb;
    
    if (uInverted) {
        color = vec3(1.0) - color; 
    }

    if (uGrayscale) {
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(gray);
    }

    vec2 offsets[9] = vec2[](
        vec2(-1,  1), vec2(0,  1), vec2(1,  1),
        vec2(-1,  0), vec2(0,  0), vec2(1,  0),
        vec2(-1, -1), vec2(0, -1), vec2(1, -1)
    );

    if (uKernel) {
        float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );

        vec3 sampleTex[9];
        for (int i = 0; i < 9; i++) {
            vec2 samplePos = textCoord + offsets[i] * uTexelSize;
            sampleTex[i] = texture(uTexture1, samplePos).rgb;
        }

        vec3 result = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            result += sampleTex[i] * kernel[i];
        }

        result = mix(color, result, uKernelStrength);
        color = result;
    }

    FragColor = vec4(color, 1.0);
}
