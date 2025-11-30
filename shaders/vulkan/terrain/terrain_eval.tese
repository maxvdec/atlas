#version 450
layout(quads, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec2 TextureCoord[];

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 FragPos;
layout(location = 2) out float Height;
layout(location = 3) out vec4 FragPosLightSpace;

layout(set = 5, binding = 0) uniform sampler2D heightMap;
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};
layout(set = 5, binding = 1) uniform Uniforms {
    mat4 lightViewProj;
    float maxPeak;
    float seaLevel;
    bool isFromMap;
};

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

    Height = texture(heightMap, texCoord).r * maxPeak - seaLevel;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 position = (p1 - p0) * v + p0;

    position.y += Height;

    gl_Position = projection * view * model * position;

    TexCoord = texCoord;
    FragPos = vec3(model * position);
    FragPosLightSpace = lightViewProj * model * position;
}