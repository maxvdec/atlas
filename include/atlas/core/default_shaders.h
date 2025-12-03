// This file contains packed shader source code.
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

static const char* FULLSCREEN_FRAG = R"(
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
const int EFFECT_DILATION = 10;
const int EFFECT_FILM_GRAIN = 11;

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
uniform sampler2D LUTTexture;
uniform sampler2D SSRTexture;
uniform int hasBrightTexture;
uniform int hasDepthTexture;
uniform int hasVolumetricLightTexture;
uniform int hasPositionTexture;
uniform int hasLUTTexture;
uniform int hasSSRTexture;
uniform float lutSize;
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
uniform mat4 invViewMatrix;
uniform mat4 lastViewMatrix;

uniform float nearPlane = 0.1;
uniform float farPlane = 100.0;

uniform float focusDepth;
uniform float focusRange;

uniform int maxMipLevel;
uniform float deltaTime;
uniform float time;

uniform sampler3D cloudsTexture;
uniform vec3 cloudPosition;
uniform vec3 cloudSize;
uniform vec3 cameraPosition;
uniform float cloudScale;
uniform vec3 cloudOffset;
uniform float cloudDensityThreshold;
uniform float cloudDensityMultiplier;
uniform float cloudAbsorption;
uniform float cloudScattering;
uniform float cloudPhaseG;
uniform float cloudClusterStrength;
uniform int cloudPrimarySteps;
uniform int cloudLightSteps;
uniform float cloudLightStepMultiplier;
uniform float cloudMinStepLength;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform vec3 cloudAmbientColor;
uniform int hasClouds;

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
        } else if (Effects[i] == EFFECT_DILATION) {
            float radius = EffectFloat1[i];
            float separation = EffectFloat2[i];
            vec2 texelSize = 1.0 / vec2(textureSize(Texture, 0));

            vec3 maxColor = texture(Texture, uv).rgb;
            int range = int(radius);
            float radiusSq = radius * radius;

            for (int x = -range; x <= range; x++) {
                for (int y = -range; y <= range; y++) {
                    float distSq = float(x * x + y * y);
                    if (distSq <= radiusSq) {
                        vec2 offset = vec2(float(x), float(y)) * texelSize * separation;
                        vec3 sampled = texture(Texture, uv + offset).rgb;
                        maxColor = max(maxColor, sampled);
                    }
                }
            }

            return vec4(maxColor, texture(Texture, uv).a);
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
        } else if (Effects[i] == EFFECT_DILATION) {
            float radius = EffectFloat1[i];
            float separation = EffectFloat2[i];
            vec2 texelSize = 1.0 / vec2(textureSize(BrightTexture, 0));

            vec3 maxColor = texture(BrightTexture, uv).rgb;
            int range = int(radius);
            float radiusSq = radius * radius;

            for (int x = -range; x <= range; x++) {
                for (int y = -range; y <= range; y++) {
                    float distSq = float(x * x + y * y);
                    if (distSq <= radiusSq) {
                        vec2 offset = vec2(float(x), float(y)) * texelSize * separation;
                        vec3 sampled = texture(BrightTexture, uv + offset).rgb;
                        maxColor = max(maxColor, sampled);
                    }
                }
            }

            return vec4(maxColor, texture(BrightTexture, uv).a);
        }
    }

    return texture(BrightTexture, uv);
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
            float levels = max(EffectFloat1[i], 1.0);
            float grayscale = max(color.r, max(color.g, color.b));
            if (grayscale > 1e-4) {
                float lower = floor(grayscale * levels) / levels;
                float lowerDiff = abs(grayscale - lower);
                float upper = ceil(grayscale * levels) / levels;
                float upperDiff = abs(upper - grayscale);
                float level = lowerDiff <= upperDiff ? lower : upper;
                float adjustment = level / max(grayscale, 1e-4);
                color = adjustment * color;
            }
        } else if (Effects[i] == EFFECT_FILM_GRAIN) {
            float amount = EffectFloat1[i];

            vec3 seed = vec3(gl_FragCoord.xy, deltaTime * 100.0);

            vec3 noise;
            float n;

            n = dot(seed, vec3(12.9898, 78.233, 45.164));
            noise.r = fract(sin(n) * 43758.5453);

            n = dot(seed, vec3(93.989, 67.345, 12.989));
            noise.g = fract(sin(n) * 28001.1234);

            n = dot(seed, vec3(39.346, 11.135, 83.155));
            noise.b = fract(sin(n) * 19283.4567);

            vec3 grain = (noise - 0.5) * 2.0 * amount;

            float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
            float visibility = 1.0 - abs(luminance - 0.5) * 0.5;

            color.rgb += grain * visibility;
            color.rgb = clamp(color.rgb, 0.0, 1.0);
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
    if (hasSSRTexture == 1) {
        fallbackColor += texture(SSRTexture, texCoord);
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
            if (hasSSRTexture == 1) {
                sampled += texture(SSRTexture, sampleCoord);
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

vec3 sampleLUT(vec3 rgb, float blueSlice, float sliceSize, float slicePixelOffset) {
    float sliceY = floor(blueSlice / lutSize);
    float sliceX = mod(blueSlice, lutSize);

    vec2 uv;
    uv.x = sliceX * sliceSize + slicePixelOffset + rgb.r * sliceSize;
    uv.y = sliceY * sliceSize + slicePixelOffset + rgb.g * sliceSize;

    return texture(LUTTexture, uv).rgb;
}

vec4 mapToLUT(vec4 color) {
    if (hasLUTTexture != 1) {
        return color;
    }

    float sliceSize = 1.0 / lutSize;
    float slicePixelOffset = sliceSize * 0.5;

    float blueIndex = color.b * (lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = min(sliceLow + 1.0, lutSize - 1.0);
    float t = blueIndex - sliceLow;

    vec3 lowColor = sampleLUT(color.rgb, sliceLow, sliceSize, slicePixelOffset);
    vec3 highColor = sampleLUT(color.rgb, sliceHigh, sliceSize, slicePixelOffset);

    vec3 finalRGB = mix(lowColor, highColor, t);

    return vec4(finalRGB, color.a);
}

vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, vec3 rayOrigin, vec3 rayDir) {
    vec3 t0 = (boundsMin - rayOrigin) / rayDir;
    vec3 t1 = (boundsMax - rayOrigin) / rayDir;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);

    float dstA = max(max(tMin.x, tMin.y), tMin.z);
    float dstB = min(tMax.x, min(tMax.y, tMax.z));

    float dstToContainer = max(0.0, dstA);
    float dstInsideContainer = max(0.0, dstB - dstToContainer);

    return vec2(dstToContainer, dstInsideContainer);
}

float saturate(float v) { return clamp(v, 0.0, 1.0); }

float hashNoise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

float henyeyGreenstein(float cosTheta, float g) {
    const float PI = 3.14159265359;
    float g2 = g * g;
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 - g2) / (4.0 * PI * max(denom, 1e-4));
}

float calculateCloudDensity(vec3 worldPos) {
    vec3 halfExtents = max(cloudSize * 0.5, vec3(1e-4));
    vec3 localPos = (worldPos - cloudPosition) / halfExtents;

    if (any(lessThan(localPos, vec3(-1.0))) ||
        any(greaterThan(localPos, vec3(1.0)))) {
        return 0.0;
    }

    vec3 uvw = localPos * 0.5 + 0.5;
    float scale = max(cloudScale, 0.001);
    vec3 noiseCoord = fract(uvw * scale + cloudOffset);

    vec4 shape = texture(cloudsTexture, noiseCoord);
    
    float cluster = saturate(cloudClusterStrength);

    float lowerFade = smoothstep(-0.95, -0.6, localPos.y);
    float upperFade = 1.0 - smoothstep(0.35, 0.95, localPos.y);
    float verticalMask = lowerFade * upperFade;

    float coverageThreshold = mix(0.6, 0.28, cluster);
    float coverageSoftness = mix(0.22, 0.34, cluster);
    float coverageNoise = mix(shape.r, shape.a, 0.4 + cluster * 0.35);
    float coverage = smoothstep(coverageThreshold,
                                coverageThreshold + coverageSoftness,
                                coverageNoise);
    coverage = pow(coverage, mix(2.0, 0.7, cluster));

    float detail = mix(smoothstep(0.25, 0.75, shape.g), 
                       smoothstep(0.2, 0.9, shape.a), 0.55);
    detail = pow(detail, mix(1.6, 0.85, cluster));

    float cavityNoise = smoothstep(0.22, 0.85, shape.b);
    float gapMask = clamp(1.0 - cavityNoise * mix(0.25, 0.7, cluster), 0.0, 1.0);

    float density = coverage * mix(detail, 1.0, cluster * 0.35);
    density = max(density * gapMask * verticalMask - 0.02, 0.0);
    density *= max(cloudDensityMultiplier, 0.0);

    return density;
}

float sampleSunTransmittance(vec3 worldPos, float stepSize) {
    vec3 lightDir = -sunDirection;
    float dirLength = length(lightDir);
    if (dirLength < 1e-3) {
        lightDir = vec3(0.0, 1.0, 0.0);
    } else {
        lightDir /= dirLength;
    }

    float maxDistance = length(cloudSize) * 1.5;
    float travel = 0.0;
    float attenuation = 1.0;
    float lightStep = max(stepSize * cloudLightStepMultiplier, cloudMinStepLength);

    int steps = max(cloudLightSteps, 1);
    for (int i = 0; i < steps && attenuation > 0.05; ++i) {
        travel += lightStep;
        if (travel > maxDistance)
            break;

        vec3 samplePos = worldPos + lightDir * travel;
        float density = calculateCloudDensity(samplePos);
        attenuation *= exp(-density * lightStep * cloudAbsorption);
        lightStep = max(lightStep * cloudLightStepMultiplier, cloudMinStepLength);
    }

    return attenuation;
}

vec4 cloudRendering(vec4 inColor) {
    if (hasClouds != 1) {
        return inColor;
    }

    float nonLinearDepth = hasDepthTexture == 1
                               ? texture(DepthTexture, TexCoord).r
                               : 1.0;
    bool depthAvailable = hasDepthTexture == 1 && nonLinearDepth < 1.0;
    float depthSample = depthAvailable ? nonLinearDepth : 1.0;

    vec3 rayOrigin = cameraPosition;

    vec4 clipSpace = vec4(TexCoord * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec4 viewSpace = invProjectionMatrix * clipSpace;
    viewSpace /= viewSpace.w;
    vec3 worldPos = (invViewMatrix * vec4(viewSpace.xyz, 1.0)).xyz;

    vec3 rayDir = normalize(worldPos - rayOrigin);

    float sceneDistance = depthAvailable ? length(worldPos - rayOrigin) : 1e6;

    vec3 boundsMin = cloudPosition - cloudSize * 0.5;
    vec3 boundsMax = cloudPosition + cloudSize * 0.5;
    vec2 rayBoxInfo = rayBoxDst(boundsMin, boundsMax, rayOrigin, rayDir);

    float distToContainer = rayBoxInfo.x;
    float distInContainer = rayBoxInfo.y;

    if (distInContainer <= 0.0) {
        return inColor;
    }

    float dstLimit = min(sceneDistance - distToContainer, distInContainer);
    dstLimit = max(dstLimit, 0.0);
    if (dstLimit <= 1e-4) {
        return inColor;
    }

    int steps = max(cloudPrimarySteps, 8);
    float baseStep = dstLimit / float(steps);
    float stepSize = max(baseStep, cloudMinStepLength);

    float jitter = hashNoise(vec3(TexCoord, time)) - 0.5;
    float travelled = clamp(jitter, -0.35, 0.35) * stepSize;
    travelled = max(travelled, 0.0);

    vec3 accumulatedLight = vec3(0.0);
    float transmittance = 1.0;

    vec3 sunDir = sunDirection;
    float sunLen = length(sunDir);
    if (sunLen > 1e-3) {
        sunDir /= sunLen;
    } else {
        sunDir = vec3(0.0, 1.0, 0.0);
    }

    float phaseG = clamp(cloudPhaseG, -0.95, 0.95);

    for (int step = 0; step < steps && travelled < dstLimit;
         ++step) {
        if (transmittance <= 0.01) {
            break;
        }

        float remainingDistance = dstLimit - travelled;
        if (remainingDistance <= 1e-5) {
            break;
        }

        float current = distToContainer + travelled;
        vec3 samplePos = rayOrigin + rayDir * current;

        float density = calculateCloudDensity(samplePos);
        if (density > 1e-4) {
            float adaptiveStep = stepSize;
            if (density < 0.02) {
                adaptiveStep = stepSize * 2.5;
            } else if (density < 0.05) {
                adaptiveStep = stepSize * 1.6;
            }
            adaptiveStep = min(adaptiveStep, remainingDistance);
            float sampleWeight = density * adaptiveStep;

            float lightTrans = sampleSunTransmittance(samplePos, adaptiveStep);
            float cosTheta = clamp(dot(rayDir, -sunDir), -1.0, 1.0);
            float phase = henyeyGreenstein(cosTheta, phaseG);

            vec3 directLight = sunColor * sunIntensity * lightTrans * phase;
            vec3 ambientLight = cloudAmbientColor;

            vec3 lighting = (ambientLight * 0.35 + directLight) *
                            sampleWeight * cloudScattering;

            accumulatedLight += lighting * transmittance;
            transmittance *= exp(-density * adaptiveStep * cloudAbsorption);
            travelled += adaptiveStep;
            continue;
        }

        float emptyAdvance = min(stepSize * 2.25, remainingDistance);
        float minAdvance = min(stepSize * 0.5, remainingDistance);
        travelled += max(emptyAdvance, minAdvance);
    }

    vec3 finalColor = inColor.rgb * transmittance + accumulatedLight;
    return vec4(clamp(finalColor, 0.0, 1.0), inColor.a);
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

    color = applyColorEffects(color);

    if (hasDepthTexture == 1) {
        float depthValue = texture(DepthTexture, TexCoord).r;
        float linearDepth = LinearizeDepth(depthValue);
        float coc = clamp(abs(linearDepth - focusDepth) / focusRange, 0.0, 1.0);
        float mip = coc * float(maxMipLevel) * 1.2;
        vec3 blurred = applyColorEffects(textureLod(Texture, TexCoord, mip)).rgb;
        vec3 sharp = color.rgb;
        color = cloudRendering(color);
    } else {
        color = cloudRendering(color);
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
        if (hasSSRTexture == 1) {
            hdrColor += texture(SSRTexture, TexCoord);
        }
    }

    hdrColor = mapToLUT(hdrColor);
    

    hdrColor.rgb = acesToneMapping(hdrColor.rgb);


    float fogFactor = 1.0 - exp(-distance * environment.fogIntensity);
    vec3 finalColor = mix(hdrColor.rgb, environment.fogColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}

)";

static const char* MAIN_FRAG = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_DEPTH_CUBE = 4;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;
const int TEXTURE_METALLIC = 9;
const int TEXTURE_ROUGHNESS = 10;
const int TEXTURE_AO = 11;
const int TEXTURE_HDR_ENVIRONMENT = 12;

const float PI = 3.14159265;

vec2 texCoord;

// ----- Structures -----
struct AmbientLight {
    vec4 color;
    float intensity;
};

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 diffuse;
    vec3 specular;
};

struct AreaLight {
    vec3 position;
    vec3 right;
    vec3 up;
    vec2 size;
    vec3 diffuse;
    vec3 specular;
    float angle;
    int castsBothSides;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    vec3 lightPos;
    bool isPointLight;
};

struct Environment {
    float rimLightIntensity;
    vec3 rimLightColor;
};

// ----- Textures -----
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;
uniform samplerCube skybox;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;

// ----- Uniforms -----
uniform int textureTypes[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;

uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;

uniform PointLight pointLights[32];
uniform int pointLightCount;

uniform SpotLight spotlights[32];
uniform int spotlightCount;

uniform AreaLight areaLights[32];
uniform int areaLightCount;

uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;
uniform bool useIBL;

uniform Environment environment;

// ----- Helper Functions -----
vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) {
            if (i == 0) color += texture(texture1, texCoord);
            else if (i == 1) color += texture(texture2, texCoord);
            else if (i == 2) color += texture(texture3, texCoord);
            else if (i == 3) color += texture(texture4, texCoord);
            else if (i == 4) color += texture(texture5, texCoord);
            else if (i == 5) color += texture(texture6, texCoord);
            else if (i == 6) color += texture(texture7, texCoord);
            else if (i == 7) color += texture(texture8, texCoord);
            else if (i == 8) color += texture(texture9, texCoord);
            else if (i == 9) color += texture(texture10, texCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 enableCubeMaps(int type, vec3 direction) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (type == i + 10) {
            if (i == 0) color += texture(cubeMap1, direction);
            else if (i == 1) color += texture(cubeMap2, direction);
            else if (i == 2) color += texture(cubeMap3, direction);
            else if (i == 3) color += texture(cubeMap4, direction);
            else if (i == 4) color += texture(cubeMap5, direction);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 sampleCubeTextureAt(int textureIndex, vec3 direction) {
    if (textureIndex == 0) return texture(cubeMap1, direction);
    else if (textureIndex == 1) return texture(cubeMap2, direction);
    else if (textureIndex == 2) return texture(cubeMap3, direction);
    else if (textureIndex == 3) return texture(cubeMap4, direction);
    else if (textureIndex == 4) return texture(cubeMap5, direction);
    return vec4(0.0);
}

vec2 getTextureDimensions(int textureIndex) {
    if (textureIndex == 0) return vec2(textureSize(texture1, 0));
    else if (textureIndex == 1) return vec2(textureSize(texture2, 0));
    else if (textureIndex == 2) return vec2(textureSize(texture3, 0));
    else if (textureIndex == 3) return vec2(textureSize(texture4, 0));
    else if (textureIndex == 4) return vec2(textureSize(texture5, 0));
    else if (textureIndex == 5) return vec2(textureSize(texture6, 0));
    else if (textureIndex == 6) return vec2(textureSize(texture7, 0));
    else if (textureIndex == 7) return vec2(textureSize(texture8, 0));
    else if (textureIndex == 8) return vec2(textureSize(texture9, 0));
    else if (textureIndex == 9) return vec2(textureSize(texture10, 0));
    return vec2(0);
}

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    else if (textureIndex == 5) return texture(texture6, uv);
    else if (textureIndex == 6) return texture(texture7, uv);
    else if (textureIndex == 7) return texture(texture8, uv);
    else if (textureIndex == 8) return texture(texture9, uv);
    else if (textureIndex == 9) return texture(texture10, uv);
    return vec4(0.0);
}

vec3 getSpecularColor() {
    vec4 specTex = enableTextures(TEXTURE_SPECULAR);
    vec3 specColor = material.albedo;
    if (specTex.r != -1.0 || specTex.g != -1.0 || specTex.b != -1.0) {
        specColor *= specTex.rgb;
    }
    return specColor;
}

vec4 applyGammaCorrection(vec4 color, float gamma) {
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec2 directionToEquirect(vec3 direction) {
    vec3 dir = normalize(direction);
    float phi = atan(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    return vec2((phi + PI) / (2.0 * PI), theta / PI);
}

vec3 sampleHDRTexture(int textureIndex, vec3 direction) {
    vec2 uv = directionToEquirect(direction);
    return sampleTextureAt(textureIndex, uv).rgb;
}

vec3 sampleEnvironmentRadiance(vec3 direction) {
    vec3 envColor = vec3(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_HDR_ENVIRONMENT) {
            envColor += sampleHDRTexture(i, direction);
            count++;
        }
    }
    if (count == 0) {
        return vec3(0.0);
    }
    return envColor / float(count);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    vec3 v = normalize(viewDir);
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    const float heightScale = 0.04;
    vec2 P = (v.xy / max(v.z, 0.05)) * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) return texCoords;
    float currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords = clamp(currentTexCoords - deltaTexCoords, vec2(0.0), vec2(1.0));
        currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return clamp(currentTexCoords, vec2(0.0), vec2(1.0));
}

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
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    return color;
}

// ----- Environment Mapping -----
vec4 getEnvironmentReflected(vec4 color) {
    vec3 I = normalize(FragPos - cameraPosition);
    vec3 R = reflect(I, normalize(Normal));
    return mix(color, vec4(texture(skybox, R).rgb, 1.0), material.reflectivity);
}

// ----- Rim Light -----
vec3 getRimLight(
    vec3 fragPos,
    vec3 N,
    vec3 V,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness
) {
    N = normalize(N);
    V = normalize(V);

    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    rim *= mix(1.2, 0.3, roughness);

    vec3 rimColor = mix(vec3(1.0), albedo, metallic) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, 0.5);

    float rimIntensity = environment.rimLightIntensity;

    vec3 rimLight = rimColor * rim * rimIntensity;

    float dist = length(cameraPosition - fragPos);
    rimLight /= (1.0 + dist * 0.1);

    return rimLight;
}

// ----- PBR -----
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 radiance, vec3 albedo, float metallic, float roughness, float reflectivity) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    return Lo;
}

vec3 calculatePBRWithAttenuation(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 radianceAttenuated, vec3 albedo, float metallic, float roughness, float reflectivity) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedo / 3.14159265 + specular) * radianceAttenuated * NdotL;
    return Lo;
}

// ----- Directional Light -----
vec3 calcAllDirectionalLights(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < directionalLightCount; i++) {
        vec3 L = normalize(-directionalLights[i].direction);
        vec3 radiance = directionalLights[i].diffuse;
        Lo += calculatePBR(N, V, L, F0, radiance, albedo, metallic, roughness, reflectivity);
    }

    return Lo;
}

// ----- Point Light -----
float calcAttenuation(PointLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);
}

vec3 calcAllPointLights(vec3 fragPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pointLightCount; i++) {
        vec3 L = pointLights[i].position - fragPos;
        float distance = length(L);

        distance = max(distance, 0.001);

        L = normalize(L);

        vec3 radiance = pointLights[i].diffuse;
        float attenuation = 1.0 / max(distance * distance, 0.01);
        vec3 radianceAttenuated = radiance * attenuation;

        vec3 H = normalize(V + L);

        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / 3.14159265 + specular) * radianceAttenuated * NdotL;
    }
    return Lo;
}

// ----- Spot Light -----
vec3 calcAllSpotLights(vec3 N, vec3 fragPos, vec3 L, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < spotlightCount; i++) {
        vec3 L = normalize(spotlights[i].position - fragPos);

        vec3 spotDirection = normalize(spotlights[i].direction);
        float theta = dot(L, -spotDirection);
        float intensity = smoothstep(spotlights[i].outerCutOff, spotlights[i].cutOff, theta);

        float distance = length(spotlights[i].position - fragPos);
        distance = max(distance, 0.001);
        float attenuation = 1.0 / max(distance * distance, 0.01);

        vec3 radiance = spotlights[i].diffuse * attenuation * intensity;

        Lo += calculatePBR(N, viewDir, L, F0, radiance, albedo, metallic, roughness, reflectivity);
    }

    return Lo;
}

// ----- Shadow Calculations -----
float calculateShadow(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDir = normalize(-directionalLights[0].direction);
    vec3 normal = normalize(Normal);
    float biasValue = shadowParam.bias;
    float bias = max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));

    int sampleCount = 0;
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = sampleTextureAt(shadowParam.textureIndex,
                    projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);

    return shadow;
}

float calculateShadowRaw(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float closestDepth = sampleTextureAt(shadowParam.textureIndex, projCoords.xy).r;

    return currentDepth > closestDepth ? 1.0 : 0.0;
}

float calculateAllShadows() {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        vec4 fragPosLightSpace = shadowParams[i].lightProjection * shadowParams[i].lightView * vec4(FragPos, 1.0);
        float shadow = calculateShadow(shadowParams[i], fragPosLightSpace);
        totalShadow = max(totalShadow, shadow);
    }
    return totalShadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos)
{
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0;

    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 54;
    const vec3 sampleOffsetDirections[] = vec3[](
            vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
            vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
            vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
            vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
            vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
            vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
            vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
            vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
            vec3(-0.1120, 0.1234, -0.7446), vec3(-0.2130, -0.0782, -0.1379),
            vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843),
            // remaining random-ish points
            vec3(0.1234, -0.5678, 0.7890), vec3(-0.6789, 0.2345, -0.4567),
            vec3(0.3456, -0.7890, 0.1234), vec3(-0.2345, 0.5678, -0.6789),
            vec3(0.7890, 0.1234, 0.5678), vec3(-0.5678, -0.6789, 0.2345),
            vec3(0.4567, 0.7890, -0.2345), vec3(-0.7890, 0.3456, -0.5678),
            vec3(0.6789, -0.2345, 0.7890), vec3(-0.1234, 0.6789, -0.4567),
            vec3(0.2345, -0.5678, 0.6789), vec3(-0.3456, 0.7890, -0.1234),
            vec3(0.5678, 0.2345, -0.7890), vec3(-0.6789, -0.5678, 0.3456),
            vec3(0.7890, -0.3456, 0.4567), vec3(-0.2345, 0.1234, -0.6789),
            vec3(0.4567, 0.7890, -0.5678), vec3(-0.5678, 0.2345, 0.6789),
            vec3(0.3456, -0.7890, -0.1234), vec3(-0.7890, 0.5678, -0.2345),
            vec3(0.6789, -0.1234, 0.3456), vec3(-0.4567, 0.7890, 0.2345),
            vec3(0.5678, -0.6789, 0.7890), vec3(-0.3456, 0.5678, -0.6789),
            vec3(0.2345, -0.7890, 0.5678), vec3(-0.6789, 0.2345, -0.1234),
            vec3(0.7890, -0.3456, -0.5678), vec3(-0.5678, 0.6789, 0.2345),
            vec3(0.4567, -0.7890, 0.3456), vec3(-0.2345, 0.1234, -0.7890),
            vec3(0.3456, -0.5678, 0.6789), vec3(-0.7890, 0.4567, -0.3456),
            vec3(0.6789, -0.1234, -0.5678), vec3(-0.4567, 0.2345, 0.7890)
        );

    for (int i = 0; i < samples; ++i)
    {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);
    return shadow;
}

float calculateAllPointShadows(vec3 fragPos) {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (shadowParams[i].isPointLight) {
            float shadow = calculatePointShadow(shadowParams[i], fragPos);
            totalShadow = max(totalShadow, shadow);
        }
    }
    return totalShadow;
}

// ----- Main -----
void main() {
    texCoord = TexCoord;

    bool hasParallaxMap = false;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            hasParallaxMap = true;
            break;
        }
    }

    if (hasParallaxMap) {
        vec3 tangentViewDir = normalize(transpose(TBN) * (cameraPosition - FragPos));
        texCoord = parallaxMapping(texCoord, tangentViewDir);
        if (texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0)
            discard;
    }

    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 N;

    if (normTexture.r != -1.0 && normTexture.g != -1.0 && normTexture.b != -1.0) {
        vec3 tangentNormal = normalize(normTexture.rgb * 2.0 - 1.0);
        N = normalize(TBN * tangentNormal);
    } else {
        N = normalize(Normal);
    }

    N = normalize(N);
    vec3 V = normalize(cameraPosition - FragPos);

    vec3 albedo = material.albedo;
    vec4 albedoTex = enableTextures(TEXTURE_COLOR);
    if (albedoTex != vec4(-1.0)) {
        albedo = albedoTex.rgb;
    }

    float metallic = material.metallic;
    vec4 metallicTex = enableTextures(TEXTURE_METALLIC);
    if (metallicTex != vec4(-1.0)) {
        metallic *= metallicTex.r;
    }

    float roughness = material.roughness;
    vec4 roughnessTex = enableTextures(TEXTURE_ROUGHNESS);
    if (roughnessTex != vec4(-1.0)) {
        roughness *= roughnessTex.r;
    }

    float ao = material.ao;
    vec4 aoTex = enableTextures(TEXTURE_AO);
    if (aoTex != vec4(-1.0)) {
        ao *= aoTex.r;
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float dirShadow = 0.0;
    float pointShadow = 0.0;

    if (shadowParamCount > 0) {
        for (int i = 0; i < shadowParamCount; i++) {
            if (!shadowParams[i].isPointLight) {
                vec4 fragPosLightSpace = shadowParams[i].lightProjection *
                        shadowParams[i].lightView *
                        vec4(FragPos, 1.0);
                dirShadow = max(dirShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
            } else {
                pointShadow = max(pointShadow, calculatePointShadow(shadowParams[i], FragPos));
            }
        }
    }

    float reflectivity = material.reflectivity;
    vec3 viewDir = normalize(cameraPosition - FragPos);

    vec3 lighting = vec3(0.0);

    lighting += calcAllDirectionalLights(N, V, albedo, metallic, roughness, F0, reflectivity) * (1.0 - dirShadow);
    lighting += calcAllPointLights(FragPos, N, V, albedo, metallic, roughness, F0, reflectivity) * (1.0 - pointShadow);
    lighting += calcAllSpotLights(N, FragPos, V, viewDir, albedo, metallic, roughness, F0, reflectivity);
    lighting += getRimLight(FragPos, N, V, F0, albedo, metallic, roughness);

    {
        vec3 areaResult = vec3(0.0);
        for (int i = 0; i < areaLightCount; ++i) {
            vec3 P = areaLights[i].position;
            vec3 R = normalize(areaLights[i].right);
            vec3 U = normalize(areaLights[i].up);
            vec2 halfSize = areaLights[i].size * 0.5;

            vec3 toPoint = FragPos - P;
            float s = clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
            float t = clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
            vec3 Q = P + R * s + U * t;

            vec3 Lvec = Q - FragPos;
            float dist = length(Lvec);
            if (dist > 0.0001) {
                vec3 L = Lvec / dist;
                vec3 Nl = normalize(cross(R, U));
                float ndotl = dot(Nl, -L);
                float facing = (areaLights[i].castsBothSides != 0) ? abs(ndotl) : max(ndotl, 0.0);
                float cosTheta = cos(radians(areaLights[i].angle));
                if (facing >= cosTheta && facing > 0.0) {
                    float attenuation = 1.0 / max(dist * dist, 0.0001);
                    vec3 radiance = areaLights[i].diffuse * attenuation * facing;
                    vec3 H = normalize(V + L);
                    float NDF = distributionGGX(N, H, roughness);
                    float G = geometrySmith(N, V, L, roughness);
                    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
                    vec3 kS = F;
                    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
                    vec3 numerator = NDF * G * F;
                    float denominator = max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);
                    vec3 specular = numerator / denominator;
                    float NdotL = max(dot(N, L), 0.0);
                    areaResult += (kD * albedo / PI + specular) * radiance * NdotL;
                }
            }
        }
        lighting += areaResult;
    }

    vec3 ambient = albedo * ambientLight.intensity * ambientLight.color.rgb * ao;

    vec3 iblContribution = vec3(0.0);
    if (useIBL) {
        vec3 irradiance = sampleEnvironmentRadiance(N);
        vec3 diffuseIBL = irradiance * albedo;

        vec3 reflection = reflect(-V, N);
        vec3 specularEnv = sampleEnvironmentRadiance(reflection);

        vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float roughnessAttenuation = mix(1.0, 0.15, clamp(roughness, 0.0, 1.0));
        vec3 specularIBL = specularEnv * roughnessAttenuation;

        iblContribution = (kD * diffuseIBL + kS * specularIBL) * ao;
    }

    vec3 color = ambient + lighting + iblContribution;

    FragColor = vec4(color, 1.0);

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(color, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor.rgb = acesToneMapping(FragColor.rgb);
}

)";

static const char* MAIN_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

out vec4 outColor;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

void main() {
    mat4 modelMatrix = model;
    if (isInstanced) {
        modelMatrix = instanceModel;
    }

    mat4 mvp = projection * view * modelMatrix;
    gl_Position = mvp * vec4(aPos, 1.0);

    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    outColor = aColor;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    vec3 N = Normal;
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    TBN = mat3(T, B, N);
}

)";

static const char* FULLSCREEN_VERT = R"(
#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}

)";

static const char* TEXT_VERT = R"(
#version 410 core
layout(location = 0) in vec4 vertex; // <vec2 pos, vec2 texture>
out vec2 texCoords;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    texCoords = vertex.zw;
}
)";

static const char* TEXT_FRAG = R"(
#version 410 core

in vec2 texCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, texCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}
)";

static const char* POINT_DEPTH_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform bool isInstanced = true;

void main() {
    if (isInstanced) {
        gl_Position = model * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = model * vec4(aPos, 1.0);
    }
}
)";

static const char* DEPTH_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 6) in mat4 instanceModel;

uniform mat4 projection; // the light space matrix
uniform mat4 view;
uniform mat4 model;
uniform bool isInstanced = true;

void main() {
    if (isInstanced) {
        gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
    } else {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
}

)";

static const char* EMPTY_FRAG = R"(
#version 410 core

void main() {}

)";

static const char* SSAO_BLUR_FRAG = R"(
#version 410 core
out float FragColor;

in vec2 TexCoord;

uniform sampler2D inSSAO;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(inSSAO, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(inSSAO, TexCoord + offset).r;
        }
    }
    FragColor = result / (4.0 * 4.0);
}
)";

static const char* POINT_DEPTH_GEOM = R"(
#version 410 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos;

void main() {
    for (int face = 0; face < 6; face++) {
        gl_Layer = face;
        for (int i = 0; i < 3; i++) {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}

)";

static const char* SSAO_FRAG = R"(
#version 410 core
out float FragColor;
in vec2 TexCoord;

uniform sampler2D gPosition;   
uniform sampler2D gNormal;
uniform sampler2D texNoise;
uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;
uniform vec2 noiseScale;

const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

void main() {
    vec3 fragPosWorld = texture(gPosition, TexCoord).xyz;
    vec3 normalWorld = texture(gNormal, TexCoord).rgb;
    
    if (length(normalWorld) < 0.001) {
        FragColor = 0.0;
        return;
    }
    
    vec3 fragPos = (view * vec4(fragPosWorld, 1.0)).xyz;
    vec3 normal = normalize((view * vec4(normalWorld, 0.0)).xyz);
    
    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz * 2.0 - 1.0);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    int validSamples = 0;
    
    for (int i = 0; i < kernelSize; i++) {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;
        
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
            continue;
        }
        
        vec3 samplePosWorld = texture(gPosition, offset.xy).xyz;
        float sampleDepth = (view * vec4(samplePosWorld, 1.0)).z;
        
        float rangeCheck = smoothstep(0.0, 1.0, radius / (abs(fragPos.z - sampleDepth) + 0.001));
        
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        validSamples++;
    }
    
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }

    FragColor = occlusion;
}
)";

static const char* POINT_DEPTH_FRAG = R"(
#version 410 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main() {
    float lightDistance = length(FragPos.xyz - lightPos);
    
    lightDistance = lightDistance / far_plane;

    gl_FragDepth = lightDistance;
}
)";

static const char* LIGHT_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* DEFERRED_FRAG = R"(
#version 410 core
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec4 gMaterial;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;
const int TEXTURE_METALLIC = 9;
const int TEXTURE_ROUGHNESS = 10;
const int TEXTURE_AO = 11;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
};

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;

uniform int textureTypes[16];
uniform int textureCount;
uniform Material material;
uniform vec3 cameraPosition;
uniform bool useTexture;
uniform bool useColor;

vec2 texCoord;

vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) {
            if (i == 0) color += texture(texture1, texCoord);
            else if (i == 1) color += texture(texture2, texCoord);
            else if (i == 2) color += texture(texture3, texCoord);
            else if (i == 3) color += texture(texture4, texCoord);
            else if (i == 4) color += texture(texture5, texCoord);
            else if (i == 5) color += texture(texture6, texCoord);
            else if (i == 6) color += texture(texture7, texCoord);
            else if (i == 7) color += texture(texture8, texCoord);
            else if (i == 8) color += texture(texture9, texCoord);
            else if (i == 9) color += texture(texture10, texCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    else if (textureIndex == 5) return texture(texture6, uv);
    else if (textureIndex == 6) return texture(texture7, uv);
    else if (textureIndex == 7) return texture(texture8, uv);
    else if (textureIndex == 8) return texture(texture9, uv);
    else if (textureIndex == 9) return texture(texture10, uv);
    return vec4(0.0);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    vec3 v = normalize(viewDir);
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    const float heightScale = 0.04;
    vec2 P = (v.xy / max(v.z, 0.05)) * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = clamp(texCoords, vec2(0.0), vec2(1.0));
    int textureIndex = -1;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) return currentTexCoords;

    float currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords = clamp(currentTexCoords - deltaTexCoords, vec2(0.0), vec2(1.0));
        currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = clamp(currentTexCoords + deltaTexCoords, vec2(0.0), vec2(1.0));
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float denom = max(afterDepth - beforeDepth, 1e-4);
    float weight = clamp(afterDepth / denom, 0.0, 1.0);
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return clamp(currentTexCoords, vec2(0.0), vec2(1.0));
}

void main() {
    texCoord = TexCoord;

    vec3 tangentViewDir = normalize(transpose(TBN) * (cameraPosition - FragPos));
    texCoord = parallaxMapping(texCoord, tangentViewDir);

    texCoord = clamp(texCoord, vec2(0.0), vec2(1.0));

    vec4 sampledColor = enableTextures(TEXTURE_COLOR);
    bool hasColorTexture = sampledColor != vec4(-1.0);

    vec4 baseColor = vec4(material.albedo, 1.0);
    vec4 albedoTex = enableTextures(TEXTURE_COLOR);
    if (albedoTex != vec4(-1.0)) {
        baseColor = albedoTex;
    }

    if (baseColor.a < 0.1)
        discard;

    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 normal;
    if (normTexture.r != -1.0 && normTexture.g != -1.0 && normTexture.b != -1.0) {
        vec3 tangentNormal = normalize(normTexture.rgb * 2.0 - 1.0);
        normal = normalize(TBN * tangentNormal);
    } else {
        normal = normalize(Normal);
    }

    vec3 albedo = baseColor.rgb;

    float metallic = material.metallic;
    vec4 metallicTex = enableTextures(TEXTURE_METALLIC);
    if (metallicTex != vec4(-1.0)) {
        metallic *= metallicTex.r;
    }

    float roughness = material.roughness;
    vec4 roughnessTex = enableTextures(TEXTURE_ROUGHNESS);
    if (roughnessTex != vec4(-1.0)) {
        roughness *= roughnessTex.r;
    }

    float ao = material.ao;
    vec4 aoTex = enableTextures(TEXTURE_AO);
    if (aoTex != vec4(-1.0)) {
        ao *= aoTex.r;
    }

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.0, 1.0);
    ao = clamp(ao, 0.0, 1.0);

    float nonlinearDepth = gl_FragCoord.z;
    gPosition = vec4(FragPos, nonlinearDepth);

    vec3 n = normalize(normal);
    if (!all(equal(n, n)) || length(n) < 1e-4) {
        n = normalize(Normal);
    }
    gNormal = vec4(n, 1.0);

    vec3 a = clamp(albedo, 0.0, 1.0);
    if (!all(equal(a, a))) {
        a = vec3(0.0);
    }
    gAlbedoSpec = vec4(a, ao);

    gMaterial = vec4(metallic, roughness, ao, 1.0);
}

)";

static const char* DEFERRED_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

out vec4 outColor;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

void main() {
    mat4 finalModel;
    if (isInstanced) {
        finalModel = instanceModel;
    } else {
        finalModel = model;
    }

    vec4 worldPos = finalModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;

    TexCoord = aTexCoord;
    outColor = aColor;

    mat3 normalMatrix = mat3(transpose(inverse(finalModel)));
    Normal = normalize(normalMatrix * aNormal);

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);
}
)";

static const char* LIGHT_FRAG = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterial;
uniform sampler2D ssao;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;
uniform samplerCube skybox;

struct AmbientLight {
    vec4 color;
    float intensity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float radius;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 diffuse;
    vec3 specular;
};

struct AreaLight {
    vec3 position;
    vec3 right;
    vec3 up;
    vec2 size;
    vec3 diffuse;
    vec3 specular;
    float angle;
    int castsBothSides;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    vec3 lightPos;
    bool isPointLight;
};

struct Environment {
    float rimLightIntensity;
    vec3 rimLightColor;
};

uniform AmbientLight ambientLight;
uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;
uniform PointLight pointLights[32];
uniform int pointLightCount;
uniform SpotLight spotlights[32];
uniform int spotlightCount;
uniform AreaLight areaLights[32];
uniform int areaLightCount;
uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;
uniform vec3 cameraPosition;
uniform bool useIBL;

uniform Environment environment;

const float PI = 3.14159265;

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    return vec4(0.0);
}

vec4 sampleCubeTextureAt(int textureIndex, vec3 direction) {
    if (textureIndex == 0) return texture(cubeMap1, direction);
    else if (textureIndex == 1) return texture(cubeMap2, direction);
    else if (textureIndex == 2) return texture(cubeMap3, direction);
    else if (textureIndex == 3) return texture(cubeMap4, direction);
    else if (textureIndex == 4) return texture(cubeMap5, direction);
    return vec4(0.0);
}

vec2 getTextureDimensions(int textureIndex) {
    if (textureIndex == 0) return vec2(textureSize(texture1, 0));
    else if (textureIndex == 1) return vec2(textureSize(texture2, 0));
    else if (textureIndex == 2) return vec2(textureSize(texture3, 0));
    else if (textureIndex == 3) return vec2(textureSize(texture4, 0));
    else if (textureIndex == 4) return vec2(textureSize(texture5, 0));
    return vec2(0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float calculateShadow(ShadowParameters shadowParam, vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = shadowParam.lightProjection * shadowParam.lightView * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDirWorld = normalize((inverse(shadowParam.lightView) * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
    float biasValue = shadowParam.bias;
    float ndotl = max(dot(normal, lightDirWorld), 0.0);
    float minBias = 0.0005;
    float bias = max(biasValue * (1.0 - ndotl), minBias);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - fragPos);
    vec2 shadowMapSize = getTextureDimensions(shadowParam.textureIndex);
    float avgDim = 0.5 * (shadowMapSize.x + shadowMapSize.y);
    float resFactor = clamp(1024.0 / max(avgDim, 1.0), 0.75, 1.25);
    float distFactor = clamp(distance / 800.0, 0.0, 1.0);
    float desiredKernel = mix(1.0, 1.5, distFactor) * resFactor;
    int kernelSize = int(clamp(floor(desiredKernel + 0.5), 1.0, 2.0));

    const vec2 poissonDisk[12] = vec2[](
            vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457),
            vec2(-0.203, 0.621), vec2(0.962, -0.195), vec2(0.473, -0.480),
            vec2(0.519, 0.767), vec2(0.185, -0.893), vec2(0.507, 0.064),
            vec2(0.896, 0.412), vec2(-0.322, -0.933), vec2(-0.792, -0.598)
        );
    float rand = fract(sin(dot(projCoords.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float angle = rand * 6.2831853;
    float ca = cos(angle), sa = sin(angle);
    mat2 rot = mat2(ca, -sa, sa, ca);

    float texelRadius = mix(1.0, 3.0, distFactor) * resFactor;
    vec2 filterRadius = texelSize * texelRadius;

    int sampleCount = 0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = rot * poissonDisk[i] * filterRadius;
        vec2 uv = projCoords.xy + offset;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }
        float pcfDepth = sampleTextureAt(shadowParam.textureIndex, uv).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        sampleCount++;
    }
    if (sampleCount > 0) {
        shadow /= float(sampleCount);
    }

    return shadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos) {
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 20;
    const vec3 sampleOffsetDirections[] = vec3[](
            vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
            vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
            vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
            vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
            vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
            vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
            vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
            vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
            vec3(-0.1120, 0.1234, -0.7446), vec3(-0.2130, -0.0782, -0.1379),
            vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843)
        );

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples);
    return shadow;
}

vec3 evaluateBRDF(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    vec3 numerator = NDF * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.0001);
    vec3 specular = numerator / denominator;

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(-light.direction);
    vec3 radiance = light.diffuse;
    return evaluateBRDF(L, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 calcPointLight(PointLight light, vec3 fragPos, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = light.position - fragPos;
    float distance = length(L);
    vec3 direction = distance > 0.0 ? (L / distance) : vec3(0.0, 0.0, 1.0);
    float attenuation = 1.0 / max(light.constant + light.linear * distance + light.quadratic * distance * distance, 0.0001);
    float fade = 1.0 - smoothstep(light.radius * 0.9, light.radius, distance);
    vec3 radiance = light.diffuse * attenuation * fade;
    return evaluateBRDF(direction, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 calcSpotLight(SpotLight light, vec3 fragPos, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = light.position - fragPos;
    float distance = length(L);
    vec3 direction = normalize(L);

    vec3 spotDirection = normalize(light.direction);
    float theta = dot(direction, -spotDirection);
    float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    vec3 radiance = light.diffuse * attenuation * intensity;
    return evaluateBRDF(direction, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 sampleEnvironmentRadiance(vec3 direction) {
    return texture(skybox, direction).rgb;
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    return color;
}

vec3 getRimLight(
    vec3 fragPos,
    vec3 N,
    vec3 V,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness
) {
    N = normalize(N);
    V = normalize(V);

    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    rim *= mix(1.2, 0.3, roughness);

    vec3 rimColor = mix(vec3(1.0), albedo, metallic) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, 0.5);

    float rimIntensity = environment.rimLightIntensity;

    vec3 rimLight = rimColor * rim * rimIntensity;

    float dist = length(cameraPosition - fragPos);
    rimLight /= (1.0 + dist * 0.1);

    return rimLight;
}

void main() {
    vec3 FragPos = texture(gPosition, TexCoord).xyz;
    vec3 N = normalize(texture(gNormal, TexCoord).xyz);
    vec4 albedoAo = texture(gAlbedoSpec, TexCoord);
    vec3 albedo = albedoAo.rgb;
    vec4 matData = texture(gMaterial, TexCoord);
    float metallic = clamp(matData.r, 0.0, 1.0);
    float roughness = clamp(matData.g, 0.0, 1.0);
    float ao = clamp(matData.b, 0.0, 1.0);

    float viewDistance = max(length(cameraPosition - FragPos), 1e-6);
    vec3 V = (cameraPosition - FragPos) / viewDistance;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float ssaoFactor = clamp(texture(ssao, TexCoord).r, 0.0, 1.0);
    float ssaoDesaturated = mix(1.0, ssaoFactor, 0.35);
    float occlusion = clamp(ao * (0.2 + 0.8 * ssaoDesaturated), 0.0, 1.0);
    float lightingOcclusion = clamp(ssaoDesaturated, 0.2, 1.0);

    float dirShadow = 0.0;
    float pointShadow = 0.0;
    for (int i = 0; i < shadowParamCount; ++i) {
        if (shadowParams[i].isPointLight) {
            pointShadow = max(pointShadow, calculatePointShadow(shadowParams[i], FragPos));
        } else {
            dirShadow = max(dirShadow, calculateShadow(shadowParams[i], FragPos, N));
        }
    }

    vec3 directionalResult = vec3(0.0);
    for (int i = 0; i < directionalLightCount; ++i) {
        directionalResult += calcDirectionalLight(directionalLights[i], N, V, F0, albedo, metallic, roughness);
    }
    directionalResult *= (1.0 - dirShadow);

    vec3 pointResult = vec3(0.0);
    for (int i = 0; i < pointLightCount; ++i) {
        pointResult += calcPointLight(pointLights[i], FragPos, N, V, F0, albedo, metallic, roughness);
    }
    pointResult *= (1.0 - pointShadow);

    vec3 spotResult = vec3(0.0);
    for (int i = 0; i < spotlightCount; ++i) {
        spotResult += calcSpotLight(spotlights[i], FragPos, N, V, F0, albedo, metallic, roughness);
    }

    vec3 areaResult = vec3(0.0);
    for (int i = 0; i < areaLightCount; ++i) {
        vec3 P = areaLights[i].position;
        vec3 R = normalize(areaLights[i].right);
        vec3 U = normalize(areaLights[i].up);
        vec2 halfSize = areaLights[i].size * 0.5;

        vec3 toPoint = FragPos - P;
        float s = clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        vec3 Q = P + R * s + U * t;

        vec3 Lvec = Q - FragPos;
        float dist = length(Lvec);
        if (dist > 0.0001) {
            vec3 L = Lvec / dist;
            vec3 Nl = normalize(cross(R, U));
            float ndotl = dot(Nl, -L);

            float facing = (areaLights[i].castsBothSides != 0) ? abs(ndotl) : max(ndotl, 0.0);
            float cosTheta = cos(radians(areaLights[i].angle));

            if (facing >= cosTheta && facing > 0.0) {
                float attenuation = 1.0 / max(dist * dist, 0.0001);
                vec3 radiance = areaLights[i].diffuse * attenuation * facing;
                areaResult += evaluateBRDF(L, radiance, N, V, F0, albedo, metallic, roughness);
            }
        }
    }

    vec3 rimResult = getRimLight(FragPos, N, V, F0, albedo, metallic, roughness);
    vec3 lighting = (directionalResult + pointResult + spotResult + areaResult + rimResult) * lightingOcclusion;

    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * albedo * occlusion;

    vec3 iblContribution = vec3(0.0);
    if (useIBL) {
        vec3 irradiance = sampleEnvironmentRadiance(N);
        vec3 diffuseIBL = irradiance * albedo;

        vec3 reflection = reflect(-V, N);
        vec3 specularEnv = sampleEnvironmentRadiance(reflection);

        vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float roughnessAttenuation = mix(1.0, 0.15, clamp(roughness, 0.0, 1.0));
        vec3 specularIBL = specularEnv * roughnessAttenuation;

        iblContribution = (kD * diffuseIBL + kS * specularIBL) * occlusion;
    }

    vec3 finalColor = ambient + lighting + iblContribution;

    if (!useIBL) {
        vec3 I = normalize(FragPos - cameraPosition);
        vec3 R = reflect(-I, N);

        vec3 F = fresnelSchlick(max(dot(N, -I), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 envColor = texture(skybox, R).rgb;
        vec3 reflection = envColor * kS;

        finalColor = mix(finalColor, reflection, F0);
    }

    FragColor = vec4(finalColor, 1.0);

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(FragColor.rgb, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    FragColor.rgb = acesToneMapping(FragColor.rgb);
}

)";

static const char* COLOR_FRAG = R"(
#version 410 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
in vec4 vertexColor;

void main() {
    vec3 color = vertexColor.rgb / (vertexColor.rgb + vec3(1.0));
    FragColor = vec4(color, vertexColor.a);
    if (length(color) > 1.0) {
        BrightColor = vec4(color, vertexColor.a);
    }
}
)";

static const char* TEXTURE_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 outColor;
out vec2 TexCoord;

void main() {
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    outColor = aColor;
}

)";

static const char* DEBUG_FRAG = R"(
#version 410 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}

)";

static const char* COLOR_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 6) in mat4 instanceModel;

out vec4 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

void main() {
    mat4 mvp;
    if (isInstanced) {
        mvp = projection * view * instanceModel;
    } else {
        mvp = projection * view * model;
    }
    gl_Position = mvp * vec4(aPos, 1.0);
    vertexColor = aColor;
}

)";

static const char* DEBUG_VERT = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

)";

static const char* TEXTURE_FRAG = R"(
#version 410 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 outColor;

uniform sampler2D textures[16];

uniform bool useTexture;
uniform bool onlyTexture;
uniform int textureCount;

vec4 calculateAllTextures() {
    vec4 color = vec4(0.0);

    for (int i = 0; i <= textureCount; i++) {
        color += texture(textures[i], TexCoord);
    }

    color /= float(textureCount + 1); 

    return color;
}

void main() {
    if (onlyTexture) {
        FragColor = calculateAllTextures(); 
        return;
    }

    if (useTexture) {
        FragColor = calculateAllTextures() * outColor;
    } else {
        FragColor = outColor;
    }
}

)";

static const char* TERRAIN_CONTROL_TESC = R"(
#version 410 core

layout(vertices = 4) out;

uniform mat4 model;
uniform mat4 view;

in vec2 TexCoord[];
out vec2 TextureCoord[];

in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
}
gl_in[gl_MaxPatchVertices];

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

    if (gl_InvocationID == 0) {
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 800;

        vec4 eyeSpacePos00 = view * model * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = view * model * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = view * model * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = view * model * gl_in[3].gl_Position;

        float dist00 = clamp((abs(eyeSpacePos00.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist01 = clamp((abs(eyeSpacePos01.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist10 = clamp((abs(eyeSpacePos10.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist11 = clamp((abs(eyeSpacePos11.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);

        float tessLevel0 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist10, dist00));
        float tessLevel1 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist00, dist01));
        float tessLevel2 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist01, dist11));
        float tessLevel3 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist11, dist10));

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}
)";

static const char* TERRAIN_FRAG = R"(
#version 410 core

in vec3 FragPos;
in float Height;
in vec2 TexCoord;
in vec4 FragPosLightSpace;  

uniform sampler2D heightMap;
uniform sampler2D moistureMap;
uniform sampler2D temperatureMap;
uniform sampler2D shadowMap;  

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;
uniform sampler2D texture11;

uniform float maxPeak;
uniform float seaLevel;
uniform bool isFromMap;

uniform vec4 directionalColor;
uniform float directionalIntensity;

struct Biome {
    int id;
    vec4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

uniform Biome biomes[12];
uniform int biomesCount;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform bool hasLight = false;
uniform bool useShadowMap = false;
uniform vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
uniform vec3 viewDir  = normalize(vec3(0.0, 1.0, 1.0));
uniform float ambientStrength = 0.3;  
const float diffuseStrength = 0.8;
const float specularStrength = 0.2;
uniform float shadowBias = 0.005;

vec4 sampleBiomeTexture(int id, vec2 uv) {
    if (id == 0) return texture(texture0, uv);
    if (id == 1) return texture(texture1, uv);
    if (id == 2) return texture(texture2, uv);
    if (id == 3) return texture(texture3, uv);
    if (id == 4) return texture(texture4, uv);
    if (id == 5) return texture(texture5, uv);
    if (id == 6) return texture(texture6, uv);
    if (id == 7) return texture(texture7, uv);
    if (id == 8) return texture(texture8, uv);
    if (id == 9) return texture(texture9, uv);
    if (id == 10) return texture(texture10, uv);
    if (id == 11) return texture(texture11, uv);
    return vec4(1,0,1,1);
}

vec4 triplanarBlend(int idx, vec3 normal, vec3 worldPos, float scale) {
    vec3 blend = abs(normal);
    blend = (blend - 0.2) * 7.0;
    blend = clamp(blend, 0.0, 1.0);
    blend /= (blend.x + blend.y + blend.z);

    vec4 xProj = sampleBiomeTexture(idx, worldPos.yz * scale);
    vec4 yProj = sampleBiomeTexture(idx, worldPos.xz * scale);
    vec4 zProj = sampleBiomeTexture(idx, worldPos.xy * scale);

    return xProj * blend.x + yProj * blend.y + zProj * blend.z;
}

vec3 calculateNormal(sampler2D heightMap, vec2 texCoord, float heightScale)
{
    float h = texture(heightMap, texCoord).r * heightScale;

    float dx = dFdx(h);
    float dy = dFdy(h);

    vec3 n = normalize(vec3(-dx, 1.0, -dy));
    return n;
}

float smoothStepRange(float value, float minV, float maxV) {
    return smoothstep(minV, maxV, value);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.1);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 acesToneMapping(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    if (biomesCount <= 0) {
        FragColor = vec4(vec3((Height + seaLevel) / maxPeak), 1.0);
        return;
    }

    float h = isFromMap ? texture(heightMap, TexCoord).r * 255.0 : (Height + seaLevel) / maxPeak * 255.0;
    float m = texture(moistureMap, TexCoord).r * 255.0;
    float t = texture(temperatureMap, TexCoord).r * 255.0;

    float texelSize = 1.0 / textureSize(heightMap, 0).x;
    float heightScale = 64.0;
    vec3 normal = calculateNormal(heightMap, TexCoord, heightScale);

    // === BIOME BLENDING ===
    vec4 baseColor = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < biomesCount; i++) {
        Biome b = biomes[i]; 

        float hW = (b.minHeight < 0.0 && b.maxHeight < 0.0) ? 1.0 : smoothStepRange(h, b.minHeight, b.maxHeight);
        float mW = (b.minMoisture < 0.0 && b.maxMoisture < 0.0) ? 1.0 : smoothStepRange(m, b.minMoisture, b.maxMoisture);
        float tW = (b.minTemperature < 0.0 && b.maxTemperature < 0.0) ? 1.0 : smoothStepRange(t, b.minTemperature, b.maxTemperature);

        float weight = (1.0 - hW) * mW * tW;
        if (weight > 0.001) {
            vec4 texColor = (b.useTexture == 1)
                ? triplanarBlend(i, normal, FragPos, 0.1)
                : b.tintColor;
            baseColor += texColor * weight;
            totalWeight += weight;
        }
    }

    baseColor /= max(totalWeight, 0.001);

    float detail = texture(heightMap, TexCoord * 64.0).r * 0.1 + 0.9;
    baseColor.rgb *= detail;

    vec3 finalColor;
    
    if (hasLight) {
        vec3 L = normalize(-lightDir);
        vec3 N = normalize(normal);
        vec3 V = normalize(viewDir);
        
        vec3 ambient = ambientStrength * baseColor.rgb;
        
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diffuseStrength * diff * directionalColor.rgb * directionalIntensity * baseColor.rgb;
        
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        vec3 specular = specularStrength * spec * directionalColor.rgb * directionalIntensity;
        
        float shadow = 0.0;
        if (useShadowMap) {
            shadow = calculateShadow(FragPosLightSpace, N);
        }
        
        finalColor = ambient + (1.0 - shadow) * (diffuse + specular);
        
    } else {
        finalColor = ambientStrength * baseColor.rgb;
    }
    
    FragColor = vec4(acesToneMapping(finalColor), 1.0);
    BrightColor = vec4(0.0);
}
)";

static const char* FLUID_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 WorldNormal;
out vec3 WorldTangent;
out vec3 WorldBitangent;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    WorldNormal = normalize(mat3(model) * aNormal);
    WorldTangent = normalize(mat3(model) * aTangent);
    WorldBitangent = normalize(mat3(model) * aBitangent);
}

)";

static const char* TERRAIN_EVAL_TESE = R"(
#version 410 core
layout(quads, fractional_odd_spacing, ccw) in;

uniform sampler2D heightMap;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightViewProj = mat4(1.0);
uniform float maxPeak = 48.0;
uniform float seaLevel = 16.0;
uniform bool isFromMap = false;

in vec2 TextureCoord[];

out float Height;
out vec2 TexCoord;
out vec3 FragPos;
out vec4 FragPosLightSpace;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

    Height = texture(heightMap, texCoord).r * maxPeak - seaLevel;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 position = (p1 - p0) * v + p0;

    position.y += Height;

    gl_Position = projection * view * model * position;

    TexCoord = texCoord;
    FragPos = vec3(model * position);
    FragPosLightSpace = lightViewProj * model * position;
}
)";

static const char* TERRAIN_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* FLUID_FRAG = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 WorldPos;          
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangent;

uniform vec4 waterColor;         
uniform sampler2D sceneTexture;  
uniform sampler2D sceneDepth;    
uniform vec3 cameraPos;           
uniform float time;
uniform float refractionStrength; 
uniform float reflectionStrength; 
uniform float depthFade;         
uniform mat4 projection;
uniform mat4 view;
uniform mat4 invProjection;      
uniform mat4 invView;
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform sampler2D movementTexture;
uniform sampler2D normalTexture;
uniform int hasNormalTexture;
uniform int hasMovementTexture;
uniform vec3 windForce;

void main()
{
    vec3 normal = normalize(WorldNormal);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    
    vec4 clipSpace = projection * view * vec4(WorldPos, 1.0);
    vec3 ndc = clipSpace.xyz / clipSpace.w;
    vec2 screenUV = ndc.xy * 0.5 + 0.5;
    
    float windStrength = length(windForce);
    vec2 windDir = windStrength > 0.001 ? normalize(windForce.xy) : vec2(1.0, 0.0);
    
    float waveSpeed = 0.15 + windStrength * 0.3;  
    float waveAmplitude = 0.01 + windStrength * 0.02;  
    float waveFrequency = 30.0 + windStrength * 10.0; 
    
    vec2 waveOffset;
    waveOffset.x = sin((TexCoord.x * windDir.x + time * waveSpeed) * waveFrequency);
    waveOffset.y = cos((TexCoord.y * windDir.y - time * waveSpeed) * waveFrequency);
    waveOffset *= waveAmplitude;
    
    vec2 flowOffset = vec2(0.0);
    if (hasMovementTexture == 1) {
        vec2 windUV = windForce.xy * time * 0.05;
        vec2 movementUV = TexCoord * 2.0 + windUV;
        vec2 movementSample = texture(movementTexture, movementUV).rg;
        
        movementSample = movementSample * 2.0 - 1.0;
        
        flowOffset = movementSample * windStrength * 0.15;
        
        waveOffset += flowOffset * 0.5;
    }
    
    if (hasNormalTexture == 1) {
        vec3 T = normalize(WorldTangent);
        vec3 B = normalize(WorldBitangent);
        vec3 N = normalize(WorldNormal);
        mat3 TBN = mat3(T, B, N);
        
        float normalSpeed = 0.03 + windStrength * 0.05; 
        vec2 normalUV1 = TexCoord * 5.0 + waveOffset * 10.0 + windForce.xy * time * normalSpeed;
        vec2 normalUV2 = TexCoord * 3.0 - waveOffset * 8.0 - windForce.xy * time * normalSpeed * 0.8;
        
        vec3 normalMap1 = texture(normalTexture, normalUV1).rgb;
        vec3 normalMap2 = texture(normalTexture, normalUV2).rgb;
        
        normalMap1 = normalMap1 * 2.0 - 1.0;
        normalMap2 = normalMap2 * 2.0 - 1.0;
        
        vec3 blendedNormal = normalize(normalMap1 + normalMap2);
        
        vec3 worldSpaceNormal = normalize(TBN * blendedNormal);
        
        float normalStrength = 0.5 + windStrength * 0.3;
        normal = normalize(mix(N, worldSpaceNormal, normalStrength));
    }
    
    vec2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;  
    reflectionUV += waveOffset * 0.3;
    
    vec2 refractionUV = screenUV - waveOffset * 0.2;
    
    bool reflectionInBounds = (reflectionUV.x >= 0.0 && reflectionUV.x <= 1.0 && 
                               reflectionUV.y >= 0.0 && reflectionUV.y <= 1.0);
    bool refractionInBounds = (refractionUV.x >= 0.0 && refractionUV.x <= 1.0 && 
                               refractionUV.y >= 0.0 && refractionUV.y <= 1.0);
    
    reflectionUV = clamp(reflectionUV, 0.05, 0.95);
    refractionUV = clamp(refractionUV, 0.05, 0.95);
    
    vec2 reflectionEdgeFade = smoothstep(0.0, 0.15, reflectionUV) * smoothstep(1.0, 0.85, reflectionUV);
    float reflectionFadeFactor = reflectionEdgeFade.x * reflectionEdgeFade.y;
    
    vec2 refractionEdgeFade = smoothstep(0.0, 0.15, refractionUV) * smoothstep(1.0, 0.85, refractionUV);
    float refractionFadeFactor = refractionEdgeFade.x * refractionEdgeFade.y;
    
    if (!reflectionInBounds) {
        reflectionFadeFactor *= 0.3;
    }
    if (!refractionInBounds) {
        refractionFadeFactor *= 0.3;
    }
    
    vec4 reflectionSample = texture(reflectionTexture, reflectionUV);
    vec4 refractionSample = texture(refractionTexture, refractionUV);
    vec4 sceneFallback = texture(sceneTexture, screenUV);
    
    bool validReflection = (length(reflectionSample.rgb) > 0.01);
    bool validRefraction = (length(refractionSample.rgb) > 0.01);
    
    if (!validReflection) {
        reflectionSample = sceneFallback;
    }
    if (!validRefraction) {
        refractionSample = sceneFallback;
    }
    
    float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 3.0);
    fresnel = mix(0.02, 1.0, fresnel); 
    fresnel *= reflectionStrength;
    
    vec3 combined = mix(refractionSample.rgb, reflectionSample.rgb, fresnel);
    
    float waterTint = clamp(depthFade * 0.3, 0.0, 0.5); 
    combined = mix(combined, waterColor.rgb, waterTint);

    float foamAmount = 0.0;
    if (hasMovementTexture == 1) {
        float foamFactor = smoothstep(0.7, 1.0, length(flowOffset) * windStrength);
        foamAmount = foamFactor * 0.2;
        combined = mix(combined, vec3(1.0), foamAmount);
    }
    
    vec3 lightDir = normalize(-lightDirection);
    
    float diffuse = max(dot(normal, lightDir), 0.0);
    diffuse = diffuse * 0.3; 
    
    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(normal, halfDir), 0.0);
    float specular = pow(specAngle, 128.0); 
    
    specular *= (fresnel * 0.5 + 0.5);
    
    float waveVariation = sin(TexCoord.x * 50.0 + time * 2.0) * 0.5 + 0.5;
    waveVariation *= cos(TexCoord.y * 50.0 - time * 1.5) * 0.5 + 0.5;
    specular *= (0.7 + waveVariation * 0.3);
    
    vec3 lighting = lightColor * (diffuse + specular * 2.0);
    combined += lighting;
    
    vec3 specularHighlight = lightColor * specular * 2.5;
    
    specularHighlight += vec3(foamAmount * 2.0);
    
    float alpha = mix(0.7, 0.95, fresnel);
    
    FragColor = vec4(combined, alpha);
    
    float luminance = max(max(combined.r, combined.g), combined.b);
    vec3 bloomColor = vec3(0.0);
    
    if (luminance > 1.0) {
        bloomColor = combined - 1.0;
    }
    
    bloomColor += specularHighlight;

    BrightColor = vec4(bloomColor, alpha);
}
)";

static const char* BLINN_PHONG_FRAG = R"(
#version 410 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec4 outColor;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_DEPTH_CUBE = 4;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;

vec2 texCoord;

// ----- Structures -----
struct AmbientLight {
    vec4 color;
    float intensity;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float reflectivity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 diffuse;
    vec3 specular;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    vec3 lightPos;
    bool isPointLight;
};

// ----- Textures -----
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;
uniform samplerCube skybox;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;

// ----- Uniforms -----
uniform int textureTypes[16];
uniform int textureCount;

uniform AmbientLight ambientLight;
uniform Material material;

uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;

uniform PointLight pointLights[32];
uniform int pointLightCount;

uniform SpotLight spotlights[32];
uniform int spotlightCount;

uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;

uniform vec3 cameraPosition;

uniform bool useTexture;
uniform bool useColor;

// ----- Helper Functions -----
vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) {
            if (i == 0) color += texture(texture1, texCoord);
            else if (i == 1) color += texture(texture2, texCoord);
            else if (i == 2) color += texture(texture3, texCoord);
            else if (i == 3) color += texture(texture4, texCoord);
            else if (i == 4) color += texture(texture5, texCoord);
            else if (i == 5) color += texture(texture6, texCoord);
            else if (i == 6) color += texture(texture7, texCoord);
            else if (i == 7) color += texture(texture8, texCoord);
            else if (i == 8) color += texture(texture9, texCoord);
            else if (i == 9) color += texture(texture10, texCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 enableCubeMaps(int type, vec3 direction) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (type == i + 10) {
            if (i == 0) color += texture(cubeMap1, direction);
            else if (i == 1) color += texture(cubeMap2, direction);
            else if (i == 2) color += texture(cubeMap3, direction);
            else if (i == 3) color += texture(cubeMap4, direction);
            else if (i == 4) color += texture(cubeMap5, direction);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 sampleCubeTextureAt(int textureIndex, vec3 direction) {
    if (textureIndex == 0) return texture(cubeMap1, direction);
    else if (textureIndex == 1) return texture(cubeMap2, direction);
    else if (textureIndex == 2) return texture(cubeMap3, direction);
    else if (textureIndex == 3) return texture(cubeMap4, direction);
    else if (textureIndex == 4) return texture(cubeMap5, direction);
    return vec4(0.0);
}

vec2 getTextureDimensions(int textureIndex) {
    if (textureIndex == 0) return vec2(textureSize(texture1, 0));
    else if (textureIndex == 1) return vec2(textureSize(texture2, 0));
    else if (textureIndex == 2) return vec2(textureSize(texture3, 0));
    else if (textureIndex == 3) return vec2(textureSize(texture4, 0));
    else if (textureIndex == 4) return vec2(textureSize(texture5, 0));
    else if (textureIndex == 5) return vec2(textureSize(texture6, 0));
    else if (textureIndex == 6) return vec2(textureSize(texture7, 0));
    else if (textureIndex == 7) return vec2(textureSize(texture8, 0));
    else if (textureIndex == 8) return vec2(textureSize(texture9, 0));
    else if (textureIndex == 9) return vec2(textureSize(texture10, 0));
    return vec2(0);
}

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    else if (textureIndex == 5) return texture(texture6, uv);
    else if (textureIndex == 6) return texture(texture7, uv);
    else if (textureIndex == 7) return texture(texture8, uv);
    else if (textureIndex == 8) return texture(texture9, uv);
    else if (textureIndex == 9) return texture(texture10, uv);
    return vec4(0.0);
}

vec3 getSpecularColor() {
    vec4 specTex = enableTextures(TEXTURE_SPECULAR);
    vec3 specColor = material.specular;
    if (specTex.r != -1.0 || specTex.g != -1.0 || specTex.b != -1.0) {
        specColor *= specTex.rgb;
    }
    return specColor;
}

vec4 applyGammaCorrection(vec4 color, float gamma) {
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    vec2 P = viewDir.xy * 0.1;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) return texCoords;
    float currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return currentTexCoords;
}

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
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    return color;
}

// ----- Environment Mapping -----
vec4 getEnvironmentReflected(vec4 color) {
    vec3 I = normalize(FragPos - cameraPosition);
    vec3 R = reflect(I, normalize(Normal));
    return mix(color, vec4(texture(skybox, R).rgb, 1.0), material.reflectivity);
}

// ----- Directional Light -----
vec3 calcDirectionalDiffuse(DirectionalLight light, vec3 norm) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * light.diffuse;
}

vec3 calcDirectionalSpecular(DirectionalLight light, vec3 norm, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(-light.direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    return spec * specColor * light.specular;
}

vec3 calcAllDirectionalLights(vec3 norm, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < directionalLightCount; i++) {
        diffuseSum += calcDirectionalDiffuse(directionalLights[i], norm);
        specularSum += calcDirectionalSpecular(directionalLights[i], norm, viewDir, specColor, material.shininess);
    }

    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Point Light -----
vec3 calcPointDiffuse(PointLight light, vec3 norm, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * light.diffuse;
}

vec3 calcPointSpecular(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    return spec * specColor * light.specular;
}

float calcAttenuation(PointLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);
}

vec3 calcAllPointLights(vec3 norm, vec3 fragPos, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < pointLightCount; i++) {
        float attenuation = calcAttenuation(pointLights[i], fragPos);
        diffuseSum += calcPointDiffuse(pointLights[i], norm, fragPos) * attenuation;
        specularSum += calcPointSpecular(pointLights[i], norm, fragPos, viewDir, specColor, material.shininess) * attenuation;
    }

    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Spot Light -----
vec3 calcSpotDiffuse(SpotLight light, vec3 norm, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 spotDirection = normalize(light.direction);
    float theta = dot(lightDir, -spotDirection);

    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);

    return diff * light.diffuse * intensity;
}

vec3 calcSpotSpecular(SpotLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 specColor, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);

    vec3 spotDirection = normalize(light.direction);
    float theta = dot(lightDir, -spotDirection);

    float intensity = smoothstep(light.outerCutOff, light.cutOff, theta);

    return spec * specColor * light.specular * intensity;
}

float calcSpotAttenuation(SpotLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (1.0 + 0.09 * distance + 0.032 * distance);
}

vec3 calcAllSpotLights(vec3 norm, vec3 fragPos, vec3 viewDir) {
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);
    vec3 specColor = getSpecularColor();

    for (int i = 0; i < spotlightCount; i++) {
        float attenuation = calcSpotAttenuation(spotlights[i], fragPos);
        diffuseSum += calcSpotDiffuse(spotlights[i], norm, fragPos) * attenuation;
        specularSum += calcSpotSpecular(spotlights[i], norm, fragPos, viewDir, specColor, material.shininess) * attenuation;
    }

    diffuseSum *= material.diffuse;
    return diffuseSum + specularSum;
}

// ----- Shadow Calculations -----
float calculateShadow(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDir = normalize(-directionalLights[0].direction);
    vec3 normal = normalize(Normal);
    float biasValue = shadowParam.bias;
    float bias = max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));

    int sampleCount = 0;
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = sampleTextureAt(shadowParam.textureIndex,
                    projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);

    return shadow;
}

float calculateShadowRaw(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float closestDepth = sampleTextureAt(shadowParam.textureIndex, projCoords.xy).r;

    return currentDepth > closestDepth ? 1.0 : 0.0;
}

float calculateAllShadows() {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        vec4 fragPosLightSpace = shadowParams[i].lightProjection * shadowParams[i].lightView * vec4(FragPos, 1.0);
        float shadow = calculateShadow(shadowParams[i], fragPosLightSpace);
        totalShadow = max(totalShadow, shadow);
    }
    return totalShadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos)
{
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0;

    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 54;
    const vec3 sampleOffsetDirections[] = vec3[](
            vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
            vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
            vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
            vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
            vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
            vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
            vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
            vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
            vec3(-0.1120, 0.1234, -0.7446), vec3(-0.2130, -0.0782, -0.1379),
            vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843),
            // remaining random-ish points
            vec3(0.1234, -0.5678, 0.7890), vec3(-0.6789, 0.2345, -0.4567),
            vec3(0.3456, -0.7890, 0.1234), vec3(-0.2345, 0.5678, -0.6789),
            vec3(0.7890, 0.1234, 0.5678), vec3(-0.5678, -0.6789, 0.2345),
            vec3(0.4567, 0.7890, -0.2345), vec3(-0.7890, 0.3456, -0.5678),
            vec3(0.6789, -0.2345, 0.7890), vec3(-0.1234, 0.6789, -0.4567),
            vec3(0.2345, -0.5678, 0.6789), vec3(-0.3456, 0.7890, -0.1234),
            vec3(0.5678, 0.2345, -0.7890), vec3(-0.6789, -0.5678, 0.3456),
            vec3(0.7890, -0.3456, 0.4567), vec3(-0.2345, 0.1234, -0.6789),
            vec3(0.4567, 0.7890, -0.5678), vec3(-0.5678, 0.2345, 0.6789),
            vec3(0.3456, -0.7890, -0.1234), vec3(-0.7890, 0.5678, -0.2345),
            vec3(0.6789, -0.1234, 0.3456), vec3(-0.4567, 0.7890, 0.2345),
            vec3(0.5678, -0.6789, 0.7890), vec3(-0.3456, 0.5678, -0.6789),
            vec3(0.2345, -0.7890, 0.5678), vec3(-0.6789, 0.2345, -0.1234),
            vec3(0.7890, -0.3456, -0.5678), vec3(-0.5678, 0.6789, 0.2345),
            vec3(0.4567, -0.7890, 0.3456), vec3(-0.2345, 0.1234, -0.7890),
            vec3(0.3456, -0.5678, 0.6789), vec3(-0.7890, 0.4567, -0.3456),
            vec3(0.6789, -0.1234, -0.5678), vec3(-0.4567, 0.2345, 0.7890)
        );

    for (int i = 0; i < samples; ++i)
    {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);
    return shadow;
}

float calculateAllPointShadows(vec3 fragPos) {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (shadowParams[i].isPointLight) {
            float shadow = calculatePointShadow(shadowParams[i], fragPos);
            totalShadow = max(totalShadow, shadow);
        }
    }
    return totalShadow;
}

// ----- Main -----
void main() {
    texCoord = TexCoord;
    vec4 baseColor;

    vec3 tangentViewDir = normalize((TBN * cameraPosition) - (TBN * FragPos));
    texCoord = parallaxMapping(texCoord, tangentViewDir);
    if (texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0)
        discard;

    if (useTexture && !useColor)
        baseColor = enableTextures(TEXTURE_COLOR);
    else if (useTexture && useColor)
        baseColor = enableTextures(TEXTURE_COLOR) * outColor;
    else if (!useTexture && useColor)
        baseColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        baseColor = vec4(1.0);

    FragColor = baseColor;
    
    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 norm = vec3(0.0);
    if (normTexture.r != -1.0 || normTexture.g != -1.0 || normTexture.b != -1.0) {
        norm = normalize(normTexture.rgb * 2.0 - 1.0);
        norm = normalize(TBN * norm);
    } else {
        norm = normalize(Normal);
    }
    vec3 viewDir = normalize(cameraPosition - FragPos);

    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * material.ambient;
    float dirShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (!shadowParams[i].isPointLight) {
            vec4 fragPosLightSpace = shadowParams[i].lightProjection *
                    shadowParams[i].lightView *
                    vec4(FragPos, 1.0);
            dirShadow = max(dirShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
        }
    }

    float pointShadow = calculateAllPointShadows(FragPos);

    vec3 directionalLights = calcAllDirectionalLights(norm, viewDir) * (1.0 - dirShadow);
    vec3 pointLights = calcAllPointLights(norm, FragPos, viewDir) * (1.0 - pointShadow);
    vec3 spotLightsContrib = calcAllSpotLights(norm, FragPos, viewDir);

    vec3 finalColor = (ambient + directionalLights + pointLights + spotLightsContrib) * baseColor.rgb;

    FragColor = vec4(finalColor, baseColor.a);
    FragColor = getEnvironmentReflected(FragColor);

    if (FragColor.a < 0.1)
        discard;

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor.rgb = acesToneMapping(FragColor.rgb);
}

)";

static const char* BINN_PHONG_VERT = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced = true;

out vec4 outColor;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

void main() {
    mat4 mvp;
    if (isInstanced) {
        mvp = projection * view * instanceModel;
    } else {
        mvp = projection * view * model;
    }
    gl_Position = mvp * vec4(aPos, 1.0);
    FragPos = vec3(instanceModel * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
    outColor = aColor;

    vec3 T = normalize(vec3(instanceModel * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(instanceModel * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(instanceModel * vec4(aNormal, 0.0)));
    TBN = mat3(T, B, N);
}

)";

static const char* SKYBOX_FRAG = R"(
#version 410 core
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
uniform vec3 sunDirection;
uniform vec4 sunColor;
uniform vec3 moonDirection;
uniform vec4 moonColor;
uniform int hasDayNight;

uniform float sunTintStrength;
uniform float moonTintStrength;
uniform float sunSizeMultiplier;
uniform float moonSizeMultiplier;
uniform float starDensity;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float layeredNoise(vec2 p) {
    float total = valueNoise(p) + valueNoise(p * 2.0) * 0.5;
    return total / 1.5;
}

float hash13(vec3 p) {
    p = fract(p * 443.897);
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

vec3 generateStars(vec3 dir, float density, float nightFactor) {
    if (density <= 0.0 || nightFactor <= 0.0) {
        return vec3(0.0);
    }
    
    vec3 starSpace = dir * 50.0;
    vec3 cell = floor(starSpace);
    vec3 localPos = fract(starSpace);
    
    float rand = hash13(cell);
    
    if (rand < density * 0.3) {
        float randX = hash13(cell + vec3(12.34, 56.78, 90.12));
        float randY = hash13(cell + vec3(23.45, 67.89, 1.23));
        float randZ = hash13(cell + vec3(34.56, 78.90, 12.34));
        
        vec3 starPos = vec3(randX, randY, randZ);
        float dist = length(localPos - starPos);
        
        float starSize = 0.02 + hash13(cell + vec3(45.67, 89.01, 23.45)) * 0.03;
        float brightness = 0.5 + hash13(cell + vec3(56.78, 90.12, 34.56)) * 0.5;
        
        float star = smoothstep(starSize, 0.0, dist) * brightness;
        float twinkle = 0.8 + 0.2 * sin(hash13(cell + vec3(67.89, 1.23, 45.67)) * 100.0);
        star *= twinkle * nightFactor;
        
        vec3 starColor = vec3(1.0);
        if (rand > 0.9) starColor = vec3(0.8, 0.9, 1.0);
        else if (rand > 0.8) starColor = vec3(1.0, 0.9, 0.8);
        
        return starColor * star;
    }
    
    return vec3(0.0);
}

vec3 generateMoonTexture(vec2 uv, float distanceFromCenter, vec3 tintColor) {
    float angle = 0.5;
    float ca = cos(angle);
    float sa = sin(angle);
    uv = vec2(ca * uv.x - sa * uv.y, sa * uv.x + ca * uv.y);
    
    float largeFeatures = valueNoise(uv * 2.0);
    largeFeatures = smoothstep(0.3, 0.7, largeFeatures);
    
    float mediumCraters = valueNoise(uv * 8.0);
    
    vec2 craterUV = uv * 6.0;
    vec2 craterCell = floor(craterUV);
    vec2 craterLocal = fract(craterUV);
    
    float craters = 1.0;
    for (int i = 0; i < 4; i++) {
        vec2 neighbor = vec2(float(i % 2), float(i / 2));
        vec2 cellPoint = craterCell + neighbor;
        
        vec2 craterCenter = vec2(hash21(cellPoint), hash21(cellPoint + vec2(13.7, 27.3)));
        float dist = length(craterLocal - neighbor - craterCenter);
        
        float craterSize = 0.15 + 0.25 * hash21(cellPoint + vec2(5.3, 9.7));
        
        if (dist < craterSize) {
            float crater = smoothstep(craterSize, craterSize * 0.3, dist);
            craters = min(craters, 1.0 - crater * 0.7);
        }
    }
    
    float surface = largeFeatures * 0.5 + mediumCraters * 0.5;
    surface *= craters;
    
    float intensity = mix(0.30, 0.75, surface);
    
    float limb = 1.0 - smoothstep(0.6, 1.0, distanceFromCenter);
    intensity *= 0.4 + 0.6 * limb;
    intensity *= 1.3;
    
    return clamp(tintColor * intensity, 0.0, 1.0);
}

void main()
{    
    vec3 dir = normalize(TexCoords);
    vec3 color = texture(skybox, TexCoords).rgb;
    
    if (hasDayNight == 1) {
        vec3 normSunDir = normalize(sunDirection);
        vec3 normMoonDir = normalize(moonDirection);
        
        float sunDot = dot(dir, normSunDir);
        float moonDot = dot(dir, normMoonDir);
        
        float nightFactor = smoothstep(0.15, -0.2, sunDirection.y);
        
        if (starDensity > 0.0) {
            color += generateStars(dir, starDensity, nightFactor);
        }
        
        float sunHorizonFade = smoothstep(-0.15, 0.05, sunDirection.y);
        
        if (sunDirection.y > -0.15) {
            float sizeAdjust = 1.0 - (sunSizeMultiplier - 1.0) * 0.001;
            float sunSize = 0.9995 * sizeAdjust;
            float sunGlowSize = 0.998 * (1.0 - (sunSizeMultiplier - 1.0) * 0.003);
            float sunHaloSize = 0.99 * (1.0 - (sunSizeMultiplier - 1.0) * 0.015);
            
            float sunDisk = smoothstep(sunSize - 0.0002, sunSize, sunDot);
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
            float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) * 
                           (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
            
            float horizonBoost = smoothstep(0.1, -0.05, sunDirection.y) * 2.0;
            sunHalo *= (0.3 + horizonBoost);
            
            color += sunColor.rgb * (sunDisk * 5.0 + sunGlow * 0.5 + sunHalo) * sunHorizonFade;
        }
        
        float moonHorizonFade = smoothstep(-0.15, 0.05, moonDirection.y);
        
        if (moonDirection.y > -0.15) {
            float sizeAdjust = 1.0 - (moonSizeMultiplier - 1.0) * 0.001;
            float moonSize = 0.9996 * sizeAdjust;
            float moonGlowSize = 0.9985 * (1.0 - (moonSizeMultiplier - 1.0) * 0.003);
            float moonHaloSize = 0.992 * (1.0 - (moonSizeMultiplier - 1.0) * 0.015);
            
            float moonDisk = smoothstep(moonSize - 0.0002, moonSize, moonDot);
            
            if (moonDisk > 0.01) {
                vec3 up = abs(normMoonDir.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
                vec3 right = normalize(cross(up, normMoonDir));
                vec3 actualUp = cross(normMoonDir, right);
                
                vec3 relativeDir = dir - normMoonDir * moonDot;
                float u = dot(relativeDir, right);
                float v = dot(relativeDir, actualUp);
                
                float distFromCenter = length(vec2(u, v)) / sqrt(1.0 - moonSize * moonSize);
                
                if (distFromCenter < 1.0) {
                    vec2 moonUV = vec2(u, v) * 200.0;
                    vec3 moonTexture = generateMoonTexture(moonUV, distFromCenter, moonColor.rgb);
                    color += moonTexture * moonDisk * 1.5 * moonHorizonFade;
                } else {
                    color += moonColor.rgb * moonDisk * 1.5 * moonHorizonFade;
                }
            }
            
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * (1.0 - moonDisk);
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * 
                            (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            
            float moonHorizonBoost = smoothstep(0.1, -0.05, moonDirection.y) * 2.0;
            moonHalo *= (0.2 + moonHorizonBoost);
            
            color += moonColor.rgb * (moonGlow * 0.3 + moonHalo) * moonHorizonFade;
        }
        
        if (sunDirection.y > -0.1 && sunTintStrength > 0.0) {
            float sunSkyInfluence = smoothstep(0.7, 0.95, sunDot) * 
                                   smoothstep(-0.1, 0.2, sunDirection.y);
            color = mix(color, color * sunColor.rgb, sunSkyInfluence * 0.3 * sunTintStrength);
            
            float sunProximity = smoothstep(0.3, 0.8, sunDot) * 
                               smoothstep(-0.1, 0.3, sunDirection.y);
            color += sunColor.rgb * sunProximity * 0.15 * sunTintStrength;
            
            float globalSunTint = smoothstep(-0.1, 0.5, sunDirection.y);
            color = mix(color, sunColor.rgb, globalSunTint * sunTintStrength * 0.08);
        }
        
        if (moonDirection.y > -0.1 && moonTintStrength > 0.0) {
            float moonSkyInfluence = smoothstep(0.8, 0.95, moonDot) * 
                                    smoothstep(-0.1, 0.2, moonDirection.y);
            color = mix(color, color * moonColor.rgb, moonSkyInfluence * 0.2 * moonTintStrength);
            
            float moonProximity = smoothstep(0.5, 0.85, moonDot) * 
                                smoothstep(-0.1, 0.3, moonDirection.y);
            color += moonColor.rgb * moonProximity * 0.08 * moonTintStrength;
            
            float globalMoonTint = smoothstep(-0.1, 0.5, moonDirection.y);
            color = mix(color, moonColor.rgb, globalMoonTint * moonTintStrength * 0.05);
        }
    }
    
    FragColor = vec4(color, 1.0);
}
)";

static const char* UPSAMPLE_FRAG = R"(
#version 410 core
uniform sampler2D srcTexture;
uniform float filterRadius;
uniform vec2 srcResolution;
in vec2 TexCoord;
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

)";

static const char* DOWNSAMPLE_FRAG = R"(
#version 410 core

uniform sampler2D srcTexture;
uniform vec2 srcResolution;

in vec2 TexCoord;
layout(location = 0) out vec3 downsample;

void main() {
    vec2 srcTexelSize = 1.0 / srcResolution;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    vec2 texCoord = TexCoord;

    vec3 a = texture(srcTexture, vec2(texCoord.x - 2 * x, texCoord.y + 2 * y)).rgb;
    vec3 b = texture(srcTexture, vec2(texCoord.x, texCoord.y + 2 * y)).rgb;
    vec3 c = texture(srcTexture, vec2(texCoord.x + 2 * x, texCoord.y + 2 * y)).rgb;

    vec3 d = texture(srcTexture, vec2(texCoord.x - 2 * x, texCoord.y)).rgb;
    vec3 e = texture(srcTexture, vec2(texCoord.x, texCoord.y)).rgb;
    vec3 f = texture(srcTexture, vec2(texCoord.x + 2 * x, texCoord.y)).rgb;

    vec3 g = texture(srcTexture, vec2(texCoord.x - 2 * x, texCoord.y - 2 * y)).rgb;
    vec3 h = texture(srcTexture, vec2(texCoord.x, texCoord.y - 2 * y)).rgb;
    vec3 i = texture(srcTexture, vec2(texCoord.x + 2 * x, texCoord.y - 2 * y)).rgb;

    vec3 j = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 k = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 l = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 m = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;
}

)";

static const char* PARTICLE_FRAG = R"(
#version 410 core
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 FragColor;
uniform sampler2D particleTexture;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(particleTexture, fragTexCoord);
        if (texColor.a < 0.01) discard;
        FragColor = texColor * fragColor;
    } else {
        vec2 center = vec2(0.5, 0.5);
        float dist = distance(fragTexCoord, center);
        
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        
        FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
        
        if (FragColor.a < 0.01) discard;
    }
}
)";

static const char* PARTICLE_VERT = R"(
#version 410 core

layout(location = 0) in vec3 quadVertex;
layout(location = 1) in vec2 texCoord;

layout(location = 2) in vec3 particlePos;
layout(location = 3) in vec4 particleColor;
layout(location = 4) in float particleSize;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform bool isAmbient;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    if (isAmbient) {
        vec4 viewParticlePos = view * vec4(particlePos, 1.0);

        vec3 viewPosition =
            viewParticlePos.xyz +
            vec3(quadVertex.x * particleSize, quadVertex.y * particleSize, 0.0);

        gl_Position = projection * model * vec4(viewPosition, 1.0);
    } else {
        vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);

        vec3 worldPosition = particlePos +
                             (quadVertex.x * cameraRight * particleSize) +
                             (quadVertex.y * cameraUp * particleSize);

        gl_Position = projection * view * vec4(worldPosition, 1.0);
    }

    fragTexCoord = texCoord;
    fragColor = particleColor;
}
)";

static const char* SKYBOX_VERT = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; 
}  

)";

static const char* GAUSSIAN_FRAG = R"(
#version 410 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D image;

uniform bool horizontal;
uniform float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
uniform float radius = 1.0;

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0) * radius; 
    vec3 result = texture(image, TexCoord).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)";

static const char* SSR_BLUR_FRAG = R"(

)";

static const char* SSR_FRAG = R"(
#version 410 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterial;
uniform sampler2D sceneColor;  
uniform sampler2D gDepth;     

uniform mat4 projection;
uniform mat4 view;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform vec3 cameraPosition;

uniform float maxDistance = 30.0;   
uniform float resolution = 0.5;     
uniform int steps = 64;            
uniform float thickness = 2.0;      
uniform float maxRoughness = 0.5;  

const float PI = 3.14159265359;

float LinearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec4 SSR(vec3 worldPos, vec3 normal, vec3 viewDir, float roughness, float metallic, vec3 albedo) {
    vec3 viewPos = (view * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize((view * vec4(normal, 0.0)).xyz);
    
    vec3 viewDirection = normalize(viewPos);
    vec3 viewReflect = normalize(reflect(viewDirection, viewNormal));
    
    if (viewReflect.z > 0.0) {
        return vec4(0.0);
    }
    
    vec3 rayOrigin = viewPos + viewNormal * 0.01;
    vec3 rayDir = viewReflect;
    
    float stepSize = maxDistance / float(steps);
    vec3 currentPos = rayOrigin;
    
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    float jitter = fract(magic.z * fract(dot(gl_FragCoord.xy, magic.xy)));
    currentPos += rayDir * stepSize * jitter;
    
    vec2 hitUV = vec2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    
    vec3 lastPos = currentPos;
    
    for (int i = 0; i < steps; i++) {
        float t = float(i) / float(steps);
        float adaptiveStep = mix(0.3, 1.0, t);
        currentPos += rayDir * stepSize * adaptiveStep;
        
        vec4 projectedPos = projection * vec4(currentPos, 1.0);
        projectedPos.xyz /= projectedPos.w;
        vec2 screenUV = projectedPos.xy * 0.5 + 0.5;
        
        if (screenUV.x < 0.0 || screenUV.x > 1.0 || 
            screenUV.y < 0.0 || screenUV.y > 1.0) {
            break;
        }
        
        vec3 sampleWorldPos = texture(gPosition, screenUV).xyz;
        vec3 sampleViewPos = (view * vec4(sampleWorldPos, 1.0)).xyz;
        
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        
        float depthDiff = sampleDepth - currentDepth;
        
        if (depthDiff > 0.0 && depthDiff < thickness) {
            vec3 binarySearchStart = lastPos;
            vec3 binarySearchEnd = currentPos;
            
            for (int j = 0; j < 8; j++) {  
                vec3 midPoint = (binarySearchStart + binarySearchEnd) * 0.5;
                
                vec4 midProj = projection * vec4(midPoint, 1.0);
                midProj.xyz /= midProj.w;
                vec2 midUV = midProj.xy * 0.5 + 0.5;
                
                vec3 midSampleWorldPos = texture(gPosition, midUV).xyz;
                vec3 midSampleViewPos = (view * vec4(midSampleWorldPos, 1.0)).xyz;
                float midSampleDepth = -midSampleViewPos.z;  
                float midCurrentDepth = -midPoint.z;        
                
                if (midCurrentDepth < midSampleDepth) {
                    binarySearchStart = midPoint;
                } else {
                    binarySearchEnd = midPoint;
                }
            }
            
            vec4 finalProj = projection * vec4(binarySearchEnd, 1.0);
            finalProj.xyz /= finalProj.w;
            hitUV = finalProj.xy * 0.5 + 0.5;
            hitDepth = length(currentPos - rayOrigin);
            hit = true;
            break;
        }
        
        lastPos = currentPos;
    }
    
    if (!hit) {
        return vec4(0.0);
    }
    
    float mipLevel = roughness * 5.0;
    vec3 reflectionColor = textureLod(sceneColor, hitUV, mipLevel).rgb;
    
    vec2 edgeFade = smoothstep(0.0, 0.15, hitUV) * (1.0 - smoothstep(0.85, 1.0, hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    
    float distanceFade = 1.0 - smoothstep(maxDistance * 0.5, maxDistance, hitDepth);
    
    float roughnessFade = 1.0 - smoothstep(0.0, maxRoughness, roughness);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 V = normalize(cameraPosition - worldPos);
    float cosTheta = max(dot(normal, V), 0.0);
    vec3 fresnel = fresnelSchlick(cosTheta, F0);
    float fresnelFactor = (fresnel.r + fresnel.g + fresnel.b) / 3.0;
    
    float finalFade = edgeFactor * distanceFade * roughnessFade * fresnelFactor;
    
    return vec4(reflectionColor, finalFade);
}

void main() {
    vec3 worldPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoord).xyz);
    vec3 albedo = texture(gAlbedoSpec, TexCoord).rgb;
    vec4 material = texture(gMaterial, TexCoord);
    
    float metallic = material.r;
    float roughness = material.g;
    
    if (length(normal) < 0.001) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    if (roughness > maxRoughness) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    vec3 viewDir = normalize(cameraPosition - worldPos);
    
    vec4 reflection = SSR(worldPos, normal, viewDir, roughness, metallic, albedo);
    
    FragColor = reflection;
}
)";

static const char* VOLUMETRIC_VERT = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}

)";

static const char* VOLUMETRIC_FRAG = R"(
#version 410 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 sunPos;

struct DirectionalLight {
    vec3 color;
};

uniform DirectionalLight directionalLight;
uniform float density;
uniform float weight;
uniform float decay;
uniform float exposure;

vec3 computeVolumetricLighting(vec2 uv) {
    vec3 color = vec3(0.0);

    vec2 deltaTexCoord = (sunPos - uv) * density;
    vec2 coord = uv;
    float illuminationDecay = 1.0;

    const int NUM_SAMPLES = 100;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        coord += deltaTexCoord;

        if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) {
            break;
        }

        vec3 sampled = texture(sceneTexture, coord).rgb;
        float brightness = dot(sampled, vec3(0.299, 0.587, 0.114));

        vec3 atmosphere = directionalLight.color * 0.02;

        if (brightness > 0.5) {
            atmosphere += sampled * 0.5;
        }

        atmosphere *= illuminationDecay * weight;
        color += atmosphere;

        illuminationDecay *= decay;
    }

    return color * 5.0;
}

void main() {
    vec3 rays = computeVolumetricLighting(TexCoords);

    FragColor = vec4(rays, 1.0);
}

)";

#endif // ATLAS_GENERATED_SHADERS_H
