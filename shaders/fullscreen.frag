#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

const int TEXTURE_COLOR = 0;
const int TEXTURE_DEPTH = 3;
const int TEXTURE_CUBE_DEPTH = 4;

const int EFFECT_INVERSION = 0;
const int EFFECT_GRAYSCALE = 1;
const int EFFECT_SHARPEN = 2;
const int EFFECT_BLUR = 3;
const int EFFECT_EDGE_DETECTION = 4;
const int EFFECT_COLOR_CORRECTION = 5;
const int EFFECT_MOTION_BLUR = 6;
const int EFFECT_CHROMATIC_ABERRATION = 7;
const int EFFECT_POSTERIZATION = 8;
const int EFFECT_PIXELATION = 9;

const float offset = 1.0 / 300.0;
const float exposure = 1.0;

vec3 reinhardToneMapping(vec3 hdrColor) {
    vec3 color = vec3(1.0) - exp(-hdrColor * 1.0);
    color = pow(color, vec3(1.0 / 2.2));
    return color;
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec4 sharpen(sampler2D image) {
    vec2 offsets[9] = vec2[](
            vec2(-offset, offset),
            vec2(0.0f, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(offset, 0.0f),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
        );

    float kernel[9] = float[](
            -1, -1, -1,
            -1, 9, -1,
            -1, -1, -1
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
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
            vec2(-offset, offset),
            vec2(0.0f, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(offset, 0.0f),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
        );

    float kernel[9] = float[](
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }

    return vec4(col, 1.0);
}

uniform sampler2D Texture;
uniform sampler2D BrightTexture;
uniform sampler2D DepthTexture;
uniform sampler2D VolumetricLightTexture;
uniform sampler2D PositionTexture;
uniform int hasBrightTexture;
uniform int hasDepthTexture;
uniform int hasVolumetricLightTexture;
uniform int hasPositionTexture;
uniform samplerCube cubeMap;
uniform bool isCubeMap;
uniform int TextureType;
uniform int EffectCount;
uniform int Effects[10];
uniform float EffectFloat1[10];
uniform float EffectFloat2[10];
uniform float EffectFloat3[10];
uniform float EffectFloat4[10];
uniform float EffectFloat5[10];
uniform float EffectFloat6[10];

struct Environment {
    vec3 fogColor;
    float fogIntensity;
};

uniform Environment environment;

uniform mat4 invProjectionMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 lastViewMatrix;

uniform float nearPlane = 0.1;
uniform float farPlane = 100.0;

uniform float focusDepth;
uniform float focusRange;

uniform int maxMipLevel;

vec4 sampleColor(vec2 uv) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_CHROMATIC_ABERRATION) {
            float redOffset = EffectFloat1[i];
            float greenOffset = EffectFloat2[i];
            float blueOffset = EffectFloat3[i];
            vec2 focusPoint = vec2(EffectFloat4[i], EffectFloat5[i]);
            vec2 sampleCoord = uv;
            vec2 direction = sampleCoord - focusPoint;
            float red = texture(Texture, sampleCoord + (direction * redOffset)).r;
            float green = texture(Texture, sampleCoord + (direction * greenOffset)).g;
            vec2 blue = texture(Texture, sampleCoord + (direction * blueOffset)).ba;
            return vec4(red, green, blue);
        } else if (Effects[i] == EFFECT_PIXELATION) {
            float pixelSizeInPixels = EffectFloat1[i];
            vec2 texSize = vec2(textureSize(Texture, 0));

            vec2 pixelSize = vec2(pixelSizeInPixels) / texSize;

            vec2 pixelated = floor(uv / pixelSize) * pixelSize;

            return texture(Texture, pixelated);
        }
    }

    return texture(Texture, uv);
}

vec4 sampleBright(vec2 uv) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_CHROMATIC_ABERRATION) {
            float redOffset = EffectFloat1[i];
            float greenOffset = EffectFloat2[i];
            float blueOffset = EffectFloat3[i];
            vec2 focusPoint = vec2(EffectFloat4[i], EffectFloat5[i]);
            vec2 sampleCoord = uv;
            vec2 direction = sampleCoord - focusPoint;
            float red = texture(BrightTexture, sampleCoord + (direction * redOffset)).r;
            float green = texture(BrightTexture, sampleCoord + (direction * greenOffset)).g;
            vec2 blue = texture(BrightTexture, sampleCoord + (direction * blueOffset)).ba;
            return vec4(red, green, blue);
        } else if (Effects[i] == EFFECT_PIXELATION) {
            float pixelSizeInPixels = EffectFloat1[i];
            vec2 texSize = vec2(textureSize(BrightTexture, 0));

            vec2 pixelSize = vec2(pixelSizeInPixels) / texSize;

            vec2 pixelated = floor(uv / pixelSize) * pixelSize;
            vec4 color = texture(BrightTexture, pixelated);
            return color;
        }
    }

    return texture(Texture, uv);
}

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    float linear = (2.0 * nearPlane * farPlane) /
            (farPlane + nearPlane - z * (farPlane - nearPlane));
    return linear / farPlane;
}

vec3 reconstructViewPos(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos = invProjectionMatrix * clipPos;
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

struct ChromaticAberrationSettings {
    float redOffset;
    float greenOffset;
    float blueOffset;
    vec2 focusPoint;
    bool enabled;
};

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

    return vec4(linearColor, color.a);
}

vec4 applyColorEffects(vec4 color) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_INVERSION) {
            color = vec4(1.0 - color.rgb, color.a);
        } else if (Effects[i] == EFFECT_GRAYSCALE) {
            float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
            color = vec4(average, average, average, color.a);
        } else if (Effects[i] == EFFECT_COLOR_CORRECTION) {
            ColorCorrection cc;
            cc.exposure = EffectFloat1[i];
            cc.contrast = EffectFloat2[i];
            cc.saturation = EffectFloat3[i];
            cc.gamma = EffectFloat4[i];
            cc.temperature = EffectFloat5[i];
            cc.tint = EffectFloat6[i];
            color = applyColorCorrection(color, cc);
        } else if (Effects[i] == EFFECT_POSTERIZATION) {
            float levels = EffectFloat1[i];
            float grayscale = max(color.r, max(color.g, color.b));
            float lower = floor(grayscale * levels) / levels;
            float lowerDiff = abs(grayscale - lower);
            float upper = ceil(grayscale * levels) / levels;
            float upperDiff = abs(upper - grayscale);
            float level = lowerDiff <= upperDiff ? lower : upper;
            float adjustment = level / grayscale;
            color = adjustment * color;
        }
    }
    return color;
}

vec4 applyFXAA(sampler2D tex, vec2 texCoord) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);

    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0 / 8.0;
    float FXAA_REDUCE_MIN = 1.0 / 128.0;

    vec3 rgbNW = sampleColor(texCoord + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = sampleColor(texCoord + vec2(1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = sampleColor(texCoord + vec2(-1.0, 1.0) * texelSize).rgb;
    vec3 rgbSE = sampleColor(texCoord + vec2(1.0, 1.0) * texelSize).rgb;
    vec3 rgbM = sampleColor(texCoord).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                dir * rcpDirMin)) * texelSize;

    vec3 rgbA = 0.5 * (
            sampleColor(texCoord + dir * (1.0 / 3.0 - 0.5)).rgb +
                sampleColor(texCoord + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
                sampleColor(texCoord + dir * -0.5).rgb +
                    sampleColor(texCoord + dir * 0.5).rgb);

    float lumaB = dot(rgbB, luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return vec4(rgbA, 1.0);
    } else {
        return vec4(rgbB, 1.0);
    }
}

vec4 applyMotionBlur(vec2 texCoord, float size, float separation, vec4 color) {
    vec4 fallbackColor = color;
    if (hasBrightTexture == 1) {
        fallbackColor += sampleBright(texCoord);
    }
    if (hasVolumetricLightTexture == 1) {
        fallbackColor += texture(VolumetricLightTexture, texCoord);
    }
    if (size <= 0.0 || separation <= 0.0) {
        return fallbackColor;
    }
    if (hasPositionTexture != 1) {
        return fallbackColor;
    }

    vec4 worldPos = texture(PositionTexture, texCoord);
    if (worldPos.a <= 0.0) {
        return fallbackColor;
    }

    vec3 viewSpacePos = (viewMatrix * worldPos).xyz;
    float distanceToCamera = length(viewSpacePos);

    if (distanceToCamera < nearPlane * 2.0) {
        return fallbackColor;
    }

    vec4 currentClipPos = projectionMatrix * viewMatrix * worldPos;
    currentClipPos.xyz /= currentClipPos.w;
    vec2 currentUV = currentClipPos.xy * 0.5 + 0.5;

    vec4 prevClipPos = projectionMatrix * lastViewMatrix * worldPos;
    prevClipPos.xyz /= prevClipPos.w;
    vec2 prevUV = prevClipPos.xy * 0.5 + 0.5;

    vec2 velocity = (currentUV - prevUV) * separation;

    float maxVelocity = 0.1;
    if (length(velocity) > maxVelocity) {
        velocity = normalize(velocity) * maxVelocity;
    }

    if (length(velocity) < 0.0001) {
        return fallbackColor;
    }

    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    int samples = int(size);
    for (int i = -samples; i <= samples; i++) {
        float t = float(i) / float(samples);
        vec2 sampleCoord = texCoord + velocity * t;

        if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 &&
                sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
            vec4 sampled = sampleColor(sampleCoord);
            if (hasBrightTexture == 1) {
                sampled += sampleBright(sampleCoord);
            }
            if (hasVolumetricLightTexture == 1) {
                sampled += texture(VolumetricLightTexture, sampleCoord);
            }

            float weight = 1.0 - abs(t) * 0.5;
            result += sampled * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0) {
        result /= totalWeight;
        return result;
    }

    return fallbackColor;
}

void main() {
    vec4 color = sampleColor(TexCoord);
    float depth = texture(DepthTexture, TexCoord).r;
    vec3 viewPos = reconstructViewPos(TexCoord, depth);
    float distance = length(viewPos);

    bool useMotionBlur = false;
    float motionBlurSize = 0.0;
    float motionBlurSeparation = 0.0;

    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_MOTION_BLUR) {
            useMotionBlur = true;
            motionBlurSize = EffectFloat1[i];
            motionBlurSeparation = EffectFloat2[i];
        }
    }

    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_SHARPEN) {
            color = sharpen(Texture);
        } else if (Effects[i] == EFFECT_BLUR) {
            float radius = EffectFloat1[i];
            color = blur(Texture, radius);
        } else if (Effects[i] == EFFECT_EDGE_DETECTION) {
            color = edgeDetection(Texture);
        }
    }

    color = applyFXAA(Texture, TexCoord);

    if (hasDepthTexture == 1) {
        float depthValue = texture(DepthTexture, TexCoord).r;
        float linearDepth = LinearizeDepth(depthValue);
        float coc = clamp(abs(linearDepth - focusDepth) / focusRange, 0.0, 1.0);
        float mip = coc * float(maxMipLevel) * 1.2;
        vec3 blurred = applyColorEffects(textureLod(Texture, TexCoord, mip)).rgb;
        vec3 sharp = color.rgb;
        color = vec4(mix(sharp, blurred, coc), 1.0);
    }

    vec4 hdrColor;
    if (useMotionBlur) {
        vec4 motionBlurred = applyMotionBlur(TexCoord, motionBlurSize, motionBlurSeparation, color);
        FragColor = motionBlurred;
        return;
    } else {
        hdrColor = color;
        if (hasBrightTexture == 1) {
            hdrColor += sampleBright(TexCoord);
        }
        if (hasVolumetricLightTexture == 1) {
            hdrColor += texture(VolumetricLightTexture, TexCoord);
        }
    }

    hdrColor.rgb = acesToneMapping(hdrColor.rgb);

    float fogFactor = 1.0 - exp(-distance * environment.fogIntensity);
    vec3 finalColor = mix(hdrColor.rgb, environment.fogColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
