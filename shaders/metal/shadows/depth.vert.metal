#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float4 gl_Position [[position, invariant]];
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
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        float4x4 _36 = _12.projection * _12.view;
        float4x4 _40 = _36 * instanceModel;
        float4 _49 = float4(in.aPos, 1.0);
        float4 _50 = _40 * _49;
        out.gl_Position = _50;
    }
    else
    {
        float4x4 _58 = _12.projection * _12.view;
        float4x4 _61 = _58 * _12.model;
        float4 _66 = float4(in.aPos, 1.0);
        float4 _67 = _61 * _66;
        out.gl_Position = _67;
    }
    return out;
}
