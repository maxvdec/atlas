#version 410 core

in vec3 FragPos;

out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord - 0.5;
    float dist = length(coord);
    float alpha = smoothstep(0.5, 0.45, dist);
    FragColor = vec4(0.0, 0.3, 0.5, alpha); 
}