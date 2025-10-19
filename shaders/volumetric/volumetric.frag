#version 410 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 sunPos;

struct DirectionalLight {
    vec2 position;
    vec3 color;
};

uniform DirectionalLight directionalLight;

uniform float density;
uniform float weight;
uniform float decay;
uniform float exposure;

float brightness(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 computeVolumetricLighting(vec2 uv) {
    vec3 color = vec3(0.0);
    vec2 deltaTexCoord = (sunPos - uv) * density;
    vec2 coord = uv;
    float illuminationDecay = 1.0;

    for (int i = 0; i < 100; i++) {
        coord += deltaTexCoord;
        vec3
        sample = texture(sceneTexture, coord). rgb;

if ( brightness(sample)> 1.0 ) {
sample *= sunColor;
}

sample *= illuminationDecay * weight;
color += sample;
illuminationDecay *= decay;
}

return color * exposure;
}

void main() {
    vec3 rays = computeVolumetricLighting(TexCoords);

    FragColor = vec4(rays, 1.0);
}
