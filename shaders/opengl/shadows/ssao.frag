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
        // Treat missing/invalid normals as fully lit (no occlusion)
        FragColor = 1.0;
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