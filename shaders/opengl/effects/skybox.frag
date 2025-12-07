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