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
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    for (int i = 0; i < 4; i++) {
        total += valueNoise(p * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return total / maxValue;
}

vec3 generateMoonTexture(vec2 uv, float distanceFromCenter, vec3 tintColor) {
    float angle = 0.5;
    mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    uv = rot * uv;
    
    float baseValue = 0.75;
    float darkValue = 0.30;
    
    float largeFeatures = layeredNoise(uv * 2.0);
    largeFeatures = smoothstep(0.3, 0.7, largeFeatures);
    
    float mediumCraters = layeredNoise(uv * 8.0);
    mediumCraters = pow(mediumCraters, 1.5);
    
    float smallDetails = valueNoise(uv * 25.0);
    
    vec2 craterUV = uv * 6.0;
    vec2 craterCell = floor(craterUV);
    vec2 craterLocal = fract(craterUV);
    
    float craters = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 cellPoint = craterCell + neighbor;
            
            vec2 craterCenter = vec2(hash21(cellPoint), hash21(cellPoint + vec2(13.7, 27.3)));
            vec2 diff = craterLocal - neighbor - craterCenter;
            float dist = length(diff);
            
            float craterSize = 0.15 + 0.25 * hash21(cellPoint + vec2(5.3, 9.7));
            
            if (dist < craterSize) {
                float crater = smoothstep(craterSize, craterSize * 0.3, dist);
                craters = min(craters, 1.0 - crater * 0.7);
            }
        }
    }
    
    float surface = largeFeatures * 0.4 + mediumCraters * 0.3 + smallDetails * 0.3;
    surface *= craters;
    
    float intensity = mix(darkValue, baseValue, surface);
    
    float colorVar = valueNoise(uv * 12.0);
    intensity += colorVar * 0.05;
    
    float limb = 1.0 - smoothstep(0.6, 1.0, distanceFromCenter);
    intensity *= 0.4 + 0.6 * limb;
    
    intensity *= 1.3;
    
    vec3 moonSurface = tintColor * intensity;
    
    return clamp(moonSurface, 0.0, 1.0);
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
        
        float sunHorizonFade = smoothstep(-0.15, 0.05, sunDirection.y);
        
        if (sunDirection.y > -0.15) {
            float baseSunSize = 0.9995;
            float baseSunGlowSize = 0.998;
            float baseSunHaloSize = 0.99;
            
            float sizeAdjust = 1.0 - (sunSizeMultiplier - 1.0) * 0.001;
            float sunSize = baseSunSize * sizeAdjust;
            float sunGlowSize = baseSunGlowSize * (1.0 - (sunSizeMultiplier - 1.0) * 0.003);
            float sunHaloSize = baseSunHaloSize * (1.0 - (sunSizeMultiplier - 1.0) * 0.015);
            
            float sunIntensity = 5.0;
            
            float sunDisk = smoothstep(sunSize - 0.0002, sunSize, sunDot);
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
            float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) * 
                           (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
            float horizonBoost = smoothstep(0.1, -0.05, sunDirection.y) * 2.0;
            sunHalo *= (0.3 + horizonBoost);
            
            color += sunColor.rgb * sunDisk * sunIntensity * sunHorizonFade;
            color += sunColor.rgb * sunGlow * 0.5 * sunHorizonFade;
            color += sunColor.rgb * sunHalo * sunHorizonFade;
        }
        
        float moonHorizonFade = smoothstep(-0.15, 0.05, moonDirection.y);
        
        if (moonDirection.y > -0.15) {
            float baseMoonSize = 0.9996;
            float baseMoonGlowSize = 0.9985;
            float baseMoonHaloSize = 0.992;
            
            float sizeAdjust = 1.0 - (moonSizeMultiplier - 1.0) * 0.001;
            float moonSize = baseMoonSize * sizeAdjust;
            float moonGlowSize = baseMoonGlowSize * (1.0 - (moonSizeMultiplier - 1.0) * 0.003);
            float moonHaloSize = baseMoonHaloSize * (1.0 - (moonSizeMultiplier - 1.0) * 0.015);
            
            float moonIntensity = 1.5;
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
                    
                    color += moonTexture * moonDisk * moonIntensity * moonHorizonFade;
                } else {
                    color += moonColor.rgb * moonDisk * moonIntensity * moonHorizonFade;
                }
            }
            
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * (1.0 - moonDisk);
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * 
                            (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            float moonHorizonBoost = smoothstep(0.1, -0.05, moonDirection.y) * 2.0;
            moonHalo *= (0.2 + moonHorizonBoost);
            
            color += moonColor.rgb * moonGlow * 0.3 * moonHorizonFade;
            color += moonColor.rgb * moonHalo * moonHorizonFade;
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