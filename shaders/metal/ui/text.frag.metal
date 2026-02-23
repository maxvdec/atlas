#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct TextColor
{
    float3 textColor;
};

struct main0_out
{
    float4 color [[color(0)]];
};

struct main0_in
{
    float2 texCoords [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant TextColor& _30 [[buffer(0)]], texture2d<float> text [[texture(0)]], sampler textSmplr [[sampler(0)]])
{
    main0_out out = {};
    float4 sampled = float4(1.0, 1.0, 1.0, text.sample(textSmplr, in.texCoords).x);
    out.color = float4(_30.textColor, 1.0) * sampled;
    return out;
}

