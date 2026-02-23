#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TextureCoord;
    float4 gl_Position;
};

struct main0_in
{
    float2 TexCoord [[attribute(0)]];
    float4 gl_Position [[attribute(1)]];
};

kernel void main0(main0_in in [[stage_in]], constant UBO& _56 [[buffer(0)]], uint gl_InvocationID [[thread_index_in_threadgroup]], uint gl_PrimitiveID [[threadgroup_position_in_grid]], device main0_out* spvOut [[buffer(28)]], constant uint* spvIndirectParams [[buffer(29)]], device MTLQuadTessellationFactorsHalf* spvTessLevel [[buffer(26)]], threadgroup main0_in* gl_in [[threadgroup(0)]])
{
    device main0_out* gl_out = &spvOut[gl_PrimitiveID * 4];
    if (gl_InvocationID < spvIndirectParams[0])
        gl_in[gl_InvocationID] = in;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (gl_InvocationID >= 4)
        return;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_out[gl_InvocationID].TextureCoord = gl_in[gl_InvocationID].TexCoord;
    if (gl_InvocationID == 0)
    {
        float4 eyeSpacePos00 = (_56.view * _56.model) * gl_in[0].gl_Position;
        float4 eyeSpacePos01 = (_56.view * _56.model) * gl_in[1].gl_Position;
        float4 eyeSpacePos10 = (_56.view * _56.model) * gl_in[2].gl_Position;
        float4 eyeSpacePos11 = (_56.view * _56.model) * gl_in[3].gl_Position;
        float dist00 = fast::clamp((abs(eyeSpacePos00.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist01 = fast::clamp((abs(eyeSpacePos01.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist10 = fast::clamp((abs(eyeSpacePos10.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist11 = fast::clamp((abs(eyeSpacePos11.z) - 20.0) / 780.0, 0.0, 1.0);
        float tessLevel0 = mix(64.0, 4.0, fast::min(dist10, dist00));
        float tessLevel1 = mix(64.0, 4.0, fast::min(dist00, dist01));
        float tessLevel2 = mix(64.0, 4.0, fast::min(dist01, dist11));
        float tessLevel3 = mix(64.0, 4.0, fast::min(dist11, dist10));
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[0] = half(tessLevel0);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[1] = half(tessLevel1);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[2] = half(tessLevel2);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[3] = half(tessLevel3);
        spvTessLevel[gl_PrimitiveID].insideTessellationFactor[0] = half(fast::max(tessLevel1, tessLevel3));
        spvTessLevel[gl_PrimitiveID].insideTessellationFactor[1] = half(fast::max(tessLevel0, tessLevel2));
    }
}

