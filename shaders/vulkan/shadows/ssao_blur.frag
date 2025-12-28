#version 450
layout(location = 0) out float FragColor;

layout(location = 0) in vec2 TexCoord;

layout(set = 2, binding = 0) uniform sampler2D inSSAO;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(inSSAO, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(inSSAO, TexCoord + offset).r;
        }
    }
    FragColor = result / 25.0;
}