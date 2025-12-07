#version 450

layout(vertices = 4) out;

layout(location = 0) in vec2 TexCoord[];
layout(location = 0) out vec2 TextureCoord[];

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};

in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
}
gl_in[gl_MaxPatchVertices];

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

    if (gl_InvocationID == 0) {
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 800;

        vec4 eyeSpacePos00 = view * model * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = view * model * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = view * model * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = view * model * gl_in[3].gl_Position;

        float dist00 = clamp((abs(eyeSpacePos00.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist01 = clamp((abs(eyeSpacePos01.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist10 = clamp((abs(eyeSpacePos10.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);
        float dist11 = clamp((abs(eyeSpacePos11.z) - MIN_DISTANCE) /
                                 (MAX_DISTANCE - MIN_DISTANCE),
                             0.0, 1.0);

        float tessLevel0 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist10, dist00));
        float tessLevel1 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist00, dist01));
        float tessLevel2 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist01, dist11));
        float tessLevel3 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL),
                               min(dist11, dist10));

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}