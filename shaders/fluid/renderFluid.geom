#version 410 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 outPosition[];
in vec3 outVelocity[];
in float outDensity[];

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Velocity;
out float Density;

uniform mat4 view;
uniform mat4 projection;
uniform float particleSize;

void main() {
    vec3 worldPos = outPosition[0];

    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);

    right *= particleSize;
    up *= particleSize;

    FragPos = worldPos;
    Velocity = outVelocity[0];
    Density = outDensity[0];

    gl_Position = projection * view * vec4(worldPos - right - up, 1.0);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

    gl_Position = projection * view * vec4(worldPos + right - up, 1.0);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = projection * view * vec4(worldPos - right + up, 1.0);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = projection * view * vec4(worldPos + right + up, 1.0);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
