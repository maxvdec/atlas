#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 WorldPos;          
in vec3 WorldNormal;      

uniform vec4 waterColor;         
uniform sampler2D sceneTexture;  
uniform sampler2D sceneDepth;    
uniform sampler2D normalMap;     
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

void main()
{
    vec3 normal = normalize(WorldNormal);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    
    vec4 clipSpace = projection * view * vec4(WorldPos, 1.0);
    vec3 ndc = clipSpace.xyz / clipSpace.w;
    vec2 screenUV = ndc.xy * 0.5 + 0.5;
    
    vec2 waveOffset;
    waveOffset.x = sin((TexCoord.x + time * 0.15) * 30.0);
    waveOffset.y = cos((TexCoord.y - time * 0.2) * 35.0);
    waveOffset *= 0.01;
    
    vec2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;  
    reflectionUV += waveOffset * 0.6;
    reflectionUV = clamp(reflectionUV, 0.002, 0.998);
    
    vec2 refractionUV = screenUV - waveOffset * 0.4;
    refractionUV = clamp(refractionUV, 0.002, 0.998);
    
    vec4 reflectionSample = texture(reflectionTexture, reflectionUV);
    vec4 refractionSample = texture(refractionTexture, refractionUV);
    
    if (length(reflectionSample.rgb) < 0.01) {
        reflectionSample = texture(sceneTexture, screenUV);
    }
    if (length(refractionSample.rgb) < 0.01) {
        refractionSample = texture(sceneTexture, screenUV);
    }
    
    float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 3.0);
    fresnel = mix(0.02, 1.0, fresnel); 
    fresnel *= reflectionStrength;
    
    vec3 combined = mix(refractionSample.rgb, reflectionSample.rgb, fresnel);

    float waterTint = clamp(depthFade * 0.3, 0.0, 0.5); 
    combined = mix(combined, waterColor.rgb, waterTint);
    
    float alpha = mix(0.7, 0.95, fresnel); 
    
    FragColor = vec4(combined, alpha);
    
    float luminance = max(max(combined.r, combined.g), combined.b);
    BrightColor = luminance > 1.0 ? vec4(combined, alpha) : vec4(0.0);
}