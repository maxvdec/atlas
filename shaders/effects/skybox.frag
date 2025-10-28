#version 410 core
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
uniform vec3 sunDirection;
uniform vec4 sunColor;
uniform vec3 moonDirection;
uniform vec4 moonColor;
uniform int hasDayNight;

void main()
{    
    vec3 dir = normalize(TexCoords);
    
    vec3 color = texture(skybox, TexCoords).rgb;
    
    if (hasDayNight == 1) {
        if (sunDirection.y > -0.1) {
            float sunSize = 0.9995;      
            float sunGlowSize = 0.998;     
            float sunIntensity = 5.0;
            
            vec3 normSunDir = normalize(sunDirection);
            float sunDot = dot(dir, normSunDir);
            
            float sunDisk = smoothstep(sunSize - 0.0002, sunSize, sunDot);
            
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * 
                           (1.0 - sunDisk);
            
            color += sunColor.rgb * sunDisk * sunIntensity;
            color += sunColor.rgb * sunGlow * 0.5;
        }
        
        if (moonDirection.y > -0.1) {
            float moonSize = 0.9996;       
            float moonGlowSize = 0.9985;   
            float moonIntensity = 1.5;
            
            vec3 normMoonDir = normalize(moonDirection);
            float moonDot = dot(dir, normMoonDir);
            
            float moonDisk = smoothstep(moonSize - 0.0002, moonSize, moonDot);
            
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * 
                            (1.0 - moonDisk);
            
            color += moonColor.rgb * moonDisk * moonIntensity;
            color += moonColor.rgb * moonGlow * 0.3;
        }
    }
    
    FragColor = vec4(color, 1.0);
}