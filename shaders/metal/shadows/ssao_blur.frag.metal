#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> inSSAO [[texture(0)]], sampler inSSAOSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 texelSize = float2(1.0) / float2(int2(inSSAO.get_width(), inSSAO.get_height()));
    float result = 0.0;
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += inSSAO.sample(inSSAOSmplr, (in.TexCoord + offset)).x;
        }
    }
    out.FragColor = result / 25.0;
    return out;
}

