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