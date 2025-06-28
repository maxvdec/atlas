#version 330 core
in vec3 fragColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(fragColor, 1.0); // Output the fragment position as color
}
