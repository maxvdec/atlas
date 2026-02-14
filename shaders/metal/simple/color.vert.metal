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
    float4 vertexColor [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
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
    float4x4 mvp;
    if (_12.isInstanced != 0u)
    {
        mvp = (_12.projection * _12.view) * instanceModel;
    }
    else
    {
        mvp = (_12.projection * _12.view) * _12.model;
    }
    out.gl_Position = mvp * float4(in.aPos, 1.0);
    out.vertexColor = in.aColor;
    return out;
}

