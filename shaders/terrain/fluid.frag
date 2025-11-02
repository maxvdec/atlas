#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 WorldPos;          
in vec3 WorldNormal;      

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
    
    vec2 waveOffset;
    waveOffset.x = sin((TexCoord.x + time * 0.15) * 30.0);
    waveOffset.y = cos((TexCoord.y - time * 0.2) * 35.0);
    waveOffset *= 0.01;
    
    vec2 flowOffset = vec2(0.0);
    if (hasMovementTexture == 1) {
        vec2 windUV = windForce.xy * time * 0.05;
        vec2 movementUV = TexCoord * 2.0 + windUV;
        vec2 movementSample = texture(movementTexture, movementUV).rg;
        
        movementSample = movementSample * 2.0 - 1.0;
        
        flowOffset = movementSample * length(windForce) * 0.15;
        
        waveOffset += flowOffset * 0.5;
    }
    
    if (hasNormalTexture == 1) {
        vec2 normalUV1 = TexCoord * 5.0 + waveOffset * 10.0 + windForce.xy * time * 0.03;
        vec2 normalUV2 = TexCoord * 3.0 - waveOffset * 8.0 - windForce.xy * time * 0.02;
        
        vec3 normalMap1 = texture(normalTexture, normalUV1).rgb;
        vec3 normalMap2 = texture(normalTexture, normalUV2).rgb;
        
        normalMap1 = normalize(normalMap1 * 2.0 - 1.0);
        normalMap2 = normalize(normalMap2 * 2.0 - 1.0);
        
        vec3 blendedNormal = normalize(normalMap1 + normalMap2);
        
        float normalStrength = 0.5 + length(windForce) * 0.3;
        normal = normalize(normal + blendedNormal * normalStrength);
    }
    
    // Reduce distortion strength for more stability
    vec2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;  
    reflectionUV += waveOffset * 0.3;
    
    vec2 refractionUV = screenUV - waveOffset * 0.2;
    
    // Check if UVs are in valid range BEFORE clamping
    bool reflectionInBounds = (reflectionUV.x >= 0.0 && reflectionUV.x <= 1.0 && 
                               reflectionUV.y >= 0.0 && reflectionUV.y <= 1.0);
    bool refractionInBounds = (refractionUV.x >= 0.0 && refractionUV.x <= 1.0 && 
                               refractionUV.y >= 0.0 && refractionUV.y <= 1.0);
    
    // Clamp UVs
    reflectionUV = clamp(reflectionUV, 0.05, 0.95);
    refractionUV = clamp(refractionUV, 0.05, 0.95);
    
    // Calculate edge fade for smooth blending
    vec2 reflectionEdgeFade = smoothstep(0.0, 0.15, reflectionUV) * smoothstep(1.0, 0.85, reflectionUV);
    float reflectionFadeFactor = reflectionEdgeFade.x * reflectionEdgeFade.y;
    
    vec2 refractionEdgeFade = smoothstep(0.0, 0.15, refractionUV) * smoothstep(1.0, 0.85, refractionUV);
    float refractionFadeFactor = refractionEdgeFade.x * refractionEdgeFade.y;
    
    // Reduce fade if out of bounds
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
        float windStrength = length(windForce);
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