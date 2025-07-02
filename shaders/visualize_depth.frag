
#version 330 core
out vec4 FragColor;
in vec2 textCoord;

uniform sampler2D uTexture1;

uniform float nearPlane = 0.1;
uniform float farPlane = 100.0;

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main() {
    float depth = texture(uTexture1, textCoord).r; 
    float linearDepth = linearizeDepth(depth);
    float normalizedDepth = linearDepth / farPlane;
    vec3 color = vec3(pow(normalizedDepth, 0.5)); 
    FragColor = vec4(color, 1.0);
}
