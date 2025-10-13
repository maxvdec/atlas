#version 330 core
out float FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

uniform float bias = 0.001;

uniform vec2 noiseScale;

const int kernelSize = 64;
const float radius = 0.5;

void main() {
    vec3 fragPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).rgb;
    vec3 randomVec = texture(texNoise, TexCoord * noiseScale).xyz;

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; i++) {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z - bias ? 1.0 : 0.0);
    }

    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = occlusion;
}