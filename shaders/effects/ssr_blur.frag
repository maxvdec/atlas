#version 410 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D inSSR;
uniform sampler2D gNormal;
uniform sampler2D gPosition;

const int KERNEL_RADIUS = 2;

void main() {
    vec4 centerSSR = texture(inSSR, TexCoord);
    vec3 centerNormal = normalize(texture(gNormal, TexCoord).xyz);
    if (!all(equal(centerNormal, centerNormal)) || length(centerNormal) < 1e-4) {
        centerNormal = vec3(0.0, 0.0, 1.0);
    }
    float centerDepth = texture(gPosition, TexCoord).w;

    vec3 colorSum = centerSSR.rgb * centerSSR.a;
    float weightSum = max(centerSSR.a, 0.0);
    float alphaSum = max(centerSSR.a, 0.0);
    int contributingSamples = centerSSR.a > 0.0 ? 1 : 0;

    vec2 texelSize = 1.0 / vec2(textureSize(inSSR, 0));

    for (int x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; ++x) {
        for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; ++y) {
            if (x == 0 && y == 0) {
                continue;
            }

            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleCoord = TexCoord + offset;

            vec4 sampleSSR = texture(inSSR, sampleCoord);
            if (sampleSSR.a <= 0.0) {
                continue;
            }

            vec3 sampleNormal = normalize(texture(gNormal, sampleCoord).xyz);
            if (!all(equal(sampleNormal, sampleNormal)) ||
                    length(sampleNormal) < 1e-4) {
                continue;
            }

            float normalWeight = max(dot(centerNormal, sampleNormal), 0.0);
            normalWeight = pow(normalWeight, 4.0);
            if (normalWeight <= 1e-4) {
                continue;
            }

            float sampleDepth = texture(gPosition, sampleCoord).w;
            float depthDiff = abs(sampleDepth - centerDepth);
            float depthWeight = exp(-depthDiff * 400.0);

            float weight = sampleSSR.a * normalWeight * depthWeight;
            if (weight <= 1e-4) {
                continue;
            }

            colorSum += sampleSSR.rgb * weight;
            weightSum += weight;
            alphaSum += sampleSSR.a;
            contributingSamples++;
        }
    }

    if (weightSum > 1e-4) {
        vec3 blurredColor = colorSum / weightSum;
        float averagedAlpha = contributingSamples > 0
                                   ? clamp(alphaSum / float(contributingSamples), 0.0, 1.0)
                                   : centerSSR.a;
        FragColor = vec4(blurredColor, max(averagedAlpha, centerSSR.a));
    } else {
        FragColor = centerSSR;
    }
}
