#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct ShadowMatrices
{
    float4x4 shadowMatrices[6];
};

struct main0_out
{
    float4 FragPos;
    float4 gl_Position;
    uint gl_Layer;
};

unknown main0_out main0(constant ShadowMatrices& _55 [[buffer(0)]])
{
    main0_out out = {};
    for (int face = 0; face < 6; face++)
    {
        out.gl_Layer = uint(face);
        for (int i = 0; i < 3; i++)
        {
            out.FragPos = _RESERVED_IDENTIFIER_FIXUP_gl_in[i].out.gl_Position;
            out.gl_Position = _55.shadowMatrices[face] * out.FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
    return out;
}

