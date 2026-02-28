#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float2 srcResolution;
    float filterRadius;
};

struct main0_out
{
    float4 upsample [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _13 [[buffer(0)]], texture2d<float> srcTexture [[texture(0)]], sampler srcTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 texelSize =
        float2(1.0) / float2(srcTexture.get_width(), srcTexture.get_height());
    float radius = clamp(_13.filterRadius, 1.0, 8.0);
    float x = radius * texelSize.x;
    float y = radius * texelSize.y;
    float2 texCoord = in.TexCoord;
    float2 step1 = float2(x, y);
    float2 step2 = step1 * 2.0;
    float3 upsampleColor = float3(0.0);
    float totalWeight = 0.0;

    float wCenter = 0.16;
    float wCardinal1 = 0.11;
    float wDiagonal1 = 0.075;
    float wCardinal2 = 0.04;
    float wDiagonal2 = 0.015;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord).xyz * wCenter;
    totalWeight += wCenter;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, step1.y)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, -step1.y)).xyz * wCardinal1;
    totalWeight += wCardinal1 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, -step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, -step1.y)).xyz * wDiagonal1;
    totalWeight += wDiagonal1 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, step2.y)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, -step2.y)).xyz * wCardinal2;
    totalWeight += wCardinal2 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, -step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, -step2.y)).xyz * wDiagonal2;
    totalWeight += wDiagonal2 * 4.0;

    upsampleColor /= float3(max(totalWeight, 1e-5));
    out.upsample = float4(upsampleColor, 1.0);
    return out;
}
