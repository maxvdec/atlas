#version 450
layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 WorldPos;
layout(location = 2) in vec3 WorldNormal;
layout(location = 3) in vec3 WorldTangent;
layout(location = 4) in vec3 WorldBitangent;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

layout(set = 2, binding = 0) uniform sampler2D sceneTexture;
layout(set = 2, binding = 1) uniform sampler2D sceneDepth;
layout(set = 2, binding = 2) uniform sampler2D reflectionTexture;
layout(set = 2, binding = 3) uniform sampler2D refractionTexture;
layout(set = 2, binding = 4) uniform sampler2D movementTexture;
layout(set = 2, binding = 5) uniform sampler2D normalTexture;

layout(set = 1, binding = 0) uniform Uniforms {
    vec4 waterColor;
    vec3 cameraPos;
    float time;
    mat4 projection;
    mat4 view;
    mat4 invProjection;
    mat4 invView;
    vec3 lightDirection;
    vec3 lightColor;
};

layout(set = 1, binding = 1) uniform Parameters {
    float refractionStrength;
    float reflectionStrength;
    float depthFade;
    int hasNormalTexture;
    int hasMovementTexture;
    vec3 windForce;
};

void main() {
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