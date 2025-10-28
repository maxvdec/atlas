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
            
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * 
                           (1.0 - sunDisk);
            
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
            
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * 
                            (1.0 - moonDisk);
            
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * 
                            (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            float moonHorizonBoost = smoothstep(0.1, -0.05, moonDirection.y) * 2.0;
            moonHalo *= (0.2 + moonHorizonBoost);
            
            color += moonColor.rgb * moonDisk * moonIntensity * moonHorizonFade;
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