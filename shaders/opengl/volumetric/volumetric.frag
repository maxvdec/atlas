#version 410 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 sunPos;

struct DirectionalLight {
    vec3 color;
};

uniform DirectionalLight directionalLight;
uniform float density;
uniform float weight;
uniform float decay;
uniform float exposure;

vec3 computeVolumetricLighting(vec2 uv) {
    vec3 color = vec3(0.0);

    vec2 deltaTexCoord = (sunPos - uv) * density;
    vec2 coord = uv;
    float illuminationDecay = 1.0;

    const int NUM_SAMPLES = 100;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        coord += deltaTexCoord;

        if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0) {
            break;
        }

        vec3 sampled = texture(sceneTexture, coord).rgb;
        float brightness = dot(sampled, vec3(0.299, 0.587, 0.114));

        vec3 atmosphere = directionalLight.color * 0.02;

        if (brightness > 0.5) {
            atmosphere += sampled * 0.5;
        }

        atmosphere *= illuminationDecay * weight;
        color += atmosphere;

        illuminationDecay *= decay;
    }

    return color * 5.0;
}

void main() {
    vec3 rays = computeVolumetricLighting(TexCoords);

    FragColor = vec4(rays, 1.0);
}
