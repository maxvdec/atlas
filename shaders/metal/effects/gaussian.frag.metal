#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    uint horizontal;
    float4 weight[5];
    float radius;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _29 [[buffer(0)]], texture2d<float> image [[texture(0)]], sampler imageSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 tex_offset = (float2(1.0) / float2(int2(image.get_width(), image.get_height()))) * _29.radius;
    float3 result = image.sample(imageSmplr, in.TexCoord).xyz * _29.weight[0].x;
    if (_29.horizontal != 0u)
    {
        for (int i = 1; i < 5; i++)
        {
            result += (image.sample(imageSmplr, (in.TexCoord + float2(tex_offset.x * float(i), 0.0))).xyz * _29.weight[i].x);
            result += (image.sample(imageSmplr, (in.TexCoord - float2(tex_offset.x * float(i), 0.0))).xyz * _29.weight[i].x);
        }
    }
    else
    {
        for (int i_1 = 1; i_1 < 5; i_1++)
        {
            result += (image.sample(imageSmplr, (in.TexCoord + float2(0.0, tex_offset.y * float(i_1)))).xyz * _29.weight[i_1].x);
            result += (image.sample(imageSmplr, (in.TexCoord - float2(0.0, tex_offset.y * float(i_1)))).xyz * _29.weight[i_1].x);
        }
    }
    out.FragColor = float4(result, 1.0);
    return out;
}

