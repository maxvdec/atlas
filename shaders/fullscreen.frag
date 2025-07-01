
#version 330 core
out vec4 FragColor;
in vec2 textCoord;

uniform sampler2D uTexture1;
uniform bool uInverted;
uniform bool uGrayscale;
uniform bool uKernel;
uniform float uKernelIntensity;
uniform bool uBlur;
uniform float uBlurIntensity;
uniform bool uEdgeDetection;
uniform vec2 uTexelSize;

void main() {
    vec3 color = texture(uTexture1, textCoord).rgb;

    vec2 offsets[9] = vec2[](
        vec2(-1,  1), vec2(0,  1), vec2(1,  1),
        vec2(-1,  0), vec2(0,  0), vec2(1,  0),
        vec2(-1, -1), vec2(0, -1), vec2(1, -1)
    );

    
    if (uBlur) {
        float kernel[9] = float[](
            1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
            2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
            1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
        );
     
        vec3 sampleTex[9];
        for (int i = 0; i < 9; i++) {
            vec2 samplePos = textCoord + offsets[i] * uTexelSize * uBlurIntensity;
            sampleTex[i] = texture(uTexture1, samplePos).rgb;
        }
    
        vec3 blurResult = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            blurResult += sampleTex[i] * kernel[i];
        }
    
        color = blurResult;
     }

    if (uEdgeDetection) {
        float edgeKernel[9] = float[](
            -1, -1, -1,
            -1,  8, -1,
            -1, -1, -1
        );

        vec3 edgeColor = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            vec2 samplePos = textCoord + offsets[i] * uTexelSize;
            vec3 sample = texture(uTexture1, samplePos).rgb;
            edgeColor += sample * edgeKernel[i];
        }

        color = mix(color, edgeColor, uKernelIntensity);
    }


    if (uKernel) {
        float sharpenKernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );

        vec3 kernelColor = vec3(0.0);
        for (int i = 0; i < 9; i++) {
            vec2 samplePos = textCoord + offsets[i] * uTexelSize;
            vec3 sample = texture(uTexture1, samplePos).rgb;
            kernelColor += sample * sharpenKernel[i];
        }

        color = mix(color, kernelColor, uKernelIntensity);
    }

    if (uGrayscale) {
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(gray);
    }

    if (uInverted) {
        color = vec3(1.0) - color;
    }

    FragColor = vec4(color, 1.0);
}

