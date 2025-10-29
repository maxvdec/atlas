#version 410 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Velocity;
in float Density;
out vec4 FragColor;

void main() {
    vec2 coord = TexCoord - 0.5;
    float dist = length(coord) * 2.0;
    
    if (dist > 1.0) {
        discard;
    }
    
    float alpha = smoothstep(1.0, 0.7, dist);
    
    if (alpha < 0.01) {
        discard;
    }
    
    vec3 color = vec3(0.2, 0.6, 0.9);
    FragColor = vec4(color * alpha, alpha);
}