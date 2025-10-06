#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

const int TEXTURE_COLOR = 0;
const int TEXTURE_DEPTH = 3;

const int EFFECT_INVERSION = 0;
const int EFFECT_GRAYSCALE = 1;
const int EFFECT_SHARPEN = 2;
const int EFFECT_BLUR = 3;
const int EFFECT_EDGE_DETECTION = 4;
const int EFFECT_COLOR_CORRECTION = 5;

const float offset = 1.0 / 300.0; 

vec4 sharpen(sampler2D image) {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }
    
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }
    
    return vec4(col, 1.0);
}

vec4 blur(sampler2D image, float radius) {
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = vec3(0.0);
    float total = 0.0;

    float sigma = radius * 0.5;
    float twoSigmaSq = 2.0 * sigma * sigma;

    for (int x = -int(radius); x <= int(radius); x++) {
        float weight = exp(-(x * x) / twoSigmaSq);
        vec2 offset = vec2(x, 0.0) * texelSize;
        result += texture(image, TexCoord + offset).rgb * weight;
        total += weight;
    }

    result /= total;

    return vec4(result, 1.0);
}

vec4 edgeDetection(sampler2D image) {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        1, 1, 1,
        1, -8, 1,
        1, 1, 1
    );

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }
    
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }
    
    return vec4(col, 1.0);
}

uniform sampler2D Texture;
uniform int TextureType;
uniform int EffectCount;
uniform int Effects[10];
uniform float EffectFloat1[10];
uniform float EffectFloat2[10];
uniform float EffectFloat3[10];
uniform float EffectFloat4[10];
uniform float EffectFloat5[10];
uniform float EffectFloat6[10];

struct ColorCorrection {
    float exposure;
    float contrast;
    float saturation;
    float gamma;
    float temperature;
    float tint;
};

vec4 applyColorCorrection(vec4 color, ColorCorrection cc) {
    vec3 linearColor = color.rgb;

    linearColor *= pow(2.0, cc.exposure);

    linearColor = (linearColor - 0.5) * cc.contrast + 0.5;

    linearColor.r += cc.temperature * 0.05;
    linearColor.g += cc.tint * 0.05;

    float luminance = dot(linearColor, vec3(0.2126, 0.7152, 0.0722));
    linearColor = mix(vec3(luminance), linearColor, cc.saturation);

    linearColor = clamp(linearColor, 0.0, 1.0);

    linearColor = pow(linearColor, vec3(1.0 / cc.gamma));

    return vec4(linearColor, color.a);
}

void main() {
    if (TextureType == TEXTURE_COLOR) {
        vec3 col = texture(Texture, TexCoord).rgb;
        FragColor = vec4(col, 1.0);
    } else if (TextureType == TEXTURE_DEPTH) {
        float depth = texture(Texture, TexCoord).r;
        FragColor = vec4(vec3(depth), 1.0);
    }

    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_INVERSION) {
            FragColor = vec4(1.0 - FragColor.rgb, FragColor.a);
        } else if (Effects[i] == EFFECT_GRAYSCALE) {
            float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
            FragColor = vec4(average, average, average, FragColor.a);
        } else if (Effects[i] == EFFECT_SHARPEN) {
            FragColor = sharpen(Texture);
        } else if (Effects[i] == EFFECT_BLUR) { 
            float radius = EffectFloat1[i];
            FragColor = blur(Texture, radius);
        } else if (Effects[i] == EFFECT_EDGE_DETECTION) {
            FragColor = edgeDetection(Texture);
        } else if (Effects[i] == EFFECT_COLOR_CORRECTION) {
            ColorCorrection cc;
            cc.exposure = EffectFloat1[i];
            cc.contrast = EffectFloat2[i];
            cc.saturation = EffectFloat3[i];
            cc.gamma = EffectFloat4[i];
            cc.temperature = EffectFloat5[i];
            cc.tint = EffectFloat6[i];
            FragColor = applyColorCorrection(FragColor, cc);
        }
    }
}
