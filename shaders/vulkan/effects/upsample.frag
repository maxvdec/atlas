#version 450

layout(set = 2, binding = 0) uniform sampler2D srcTexture;
layout(set = 1, binding = 0) uniform Params {
    vec2 srcResolution;
    float filterRadius;
};

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec3 upsample;

void main() {
    vec2 texelSize = 1.0 / srcResolution;
    float x = filterRadius * texelSize.x;
    float y = filterRadius * texelSize.y;

    vec2 texCoord = TexCoord;
    vec3 a = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(texCoord.x, texCoord.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 d = texture(srcTexture, vec2(texCoord.x - x, texCoord.y)).rgb;
    vec3 e = texture(srcTexture, vec2(texCoord.x, texCoord.y)).rgb;
    vec3 f = texture(srcTexture, vec2(texCoord.x + x, texCoord.y)).rgb;
    vec3 g = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(texCoord.x, texCoord.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;
}
