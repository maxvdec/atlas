#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    uint useTexture;
    uint onlyTexture;
    int textureCount;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float4 outColor [[user(locn1)]];
};

static inline __attribute__((always_inline))
float4 calculateAllTextures(constant UBO& _28, texture2d<float> texture1, sampler texture1Smplr, thread float2& TexCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr, texture2d<float> texture12, sampler texture12Smplr, texture2d<float> texture13, sampler texture13Smplr, texture2d<float> texture14, sampler texture14Smplr, texture2d<float> texture15, sampler texture15Smplr, texture2d<float> texture16, sampler texture16Smplr)
{
    float4 color = float4(0.0);
    for (int i = 0; i < _28.textureCount; i++)
    {
        if (i == 0)
        {
            color += texture1.sample(texture1Smplr, TexCoord);
        }
        else
        {
            if (i == 1)
            {
                color += texture2.sample(texture2Smplr, TexCoord);
            }
            else
            {
                if (i == 2)
                {
                    color += texture3.sample(texture3Smplr, TexCoord);
                }
                else
                {
                    if (i == 3)
                    {
                        color += texture4.sample(texture4Smplr, TexCoord);
                    }
                    else
                    {
                        if (i == 4)
                        {
                            color += texture5.sample(texture5Smplr, TexCoord);
                        }
                        else
                        {
                            if (i == 5)
                            {
                                color += texture6.sample(texture6Smplr, TexCoord);
                            }
                            else
                            {
                                if (i == 6)
                                {
                                    color += texture7.sample(texture7Smplr, TexCoord);
                                }
                                else
                                {
                                    if (i == 7)
                                    {
                                        color += texture8.sample(texture8Smplr, TexCoord);
                                    }
                                    else
                                    {
                                        if (i == 8)
                                        {
                                            color += texture9.sample(texture9Smplr, TexCoord);
                                        }
                                        else
                                        {
                                            if (i == 9)
                                            {
                                                color += texture10.sample(texture10Smplr, TexCoord);
                                            }
                                            else
                                            {
                                                if (i == 10)
                                                {
                                                    color += texture11.sample(texture11Smplr, TexCoord);
                                                }
                                                else
                                                {
                                                    if (i == 11)
                                                    {
                                                        color += texture12.sample(texture12Smplr, TexCoord);
                                                    }
                                                    else
                                                    {
                                                        if (i == 12)
                                                        {
                                                            color += texture13.sample(texture13Smplr, TexCoord);
                                                        }
                                                        else
                                                        {
                                                            if (i == 13)
                                                            {
                                                                color += texture14.sample(texture14Smplr, TexCoord);
                                                            }
                                                            else
                                                            {
                                                                if (i == 14)
                                                                {
                                                                    color += texture15.sample(texture15Smplr, TexCoord);
                                                                }
                                                                else
                                                                {
                                                                    if (i == 15)
                                                                    {
                                                                        color += texture16.sample(texture16Smplr, TexCoord);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    color /= float4(float(_28.textureCount));
    return color;
}

fragment main0_out main0(main0_in in [[stage_in]], constant UBO& _28 [[buffer(0)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], texture2d<float> texture11 [[texture(10)]], texture2d<float> texture12 [[texture(11)]], texture2d<float> texture13 [[texture(12)]], texture2d<float> texture14 [[texture(13)]], texture2d<float> texture15 [[texture(14)]], texture2d<float> texture16 [[texture(15)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], sampler texture11Smplr [[sampler(10)]], sampler texture12Smplr [[sampler(11)]], sampler texture13Smplr [[sampler(12)]], sampler texture14Smplr [[sampler(13)]], sampler texture15Smplr [[sampler(14)]], sampler texture16Smplr [[sampler(15)]])
{
    main0_out out = {};
    if (_28.onlyTexture != 0u)
    {
        out.FragColor = calculateAllTextures(_28, texture1, texture1Smplr, in.TexCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr, texture12, texture12Smplr, texture13, texture13Smplr, texture14, texture14Smplr, texture15, texture15Smplr, texture16, texture16Smplr);
        return out;
    }
    if (_28.useTexture != 0u)
    {
        out.FragColor = calculateAllTextures(_28, texture1, texture1Smplr, in.TexCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr, texture12, texture12Smplr, texture13, texture13Smplr, texture14, texture14Smplr, texture15, texture15Smplr, texture16, texture16Smplr) * in.outColor;
    }
    else
    {
        out.FragColor = in.outColor;
    }
    return out;
}

