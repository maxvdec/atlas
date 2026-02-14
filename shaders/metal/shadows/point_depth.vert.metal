#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    uint isInstanced;
};

struct main0_out
{
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    if (_12.isInstanced != 0u)
    {
        out.gl_Position = (_12.model * instanceModel) * float4(in.aPos, 1.0);
    }
    else
    {
        out.gl_Position = _12.model * float4(in.aPos, 1.0);
    }
    return out;
}

