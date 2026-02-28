#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float2 srcResolution;
};

struct main0_out
{
    float4 downsample [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _13 [[buffer(0)]], texture2d<float> srcTexture [[texture(0)]], sampler srcTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 srcTexelSize =
        float2(1.0) / float2(srcTexture.get_width(), srcTexture.get_height());
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    float2 texCoord = in.TexCoord;
    float3 a = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 b = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y + (2.0 * y))).xyz;
    float3 c = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 d = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y)).xyz;
    float3 e = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y)).xyz;
    float3 f = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y)).xyz;
    float3 g = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 h = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y - (2.0 * y))).xyz;
    float3 i = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 j = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y + y)).xyz;
    float3 k = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y + y)).xyz;
    float3 l = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y - y)).xyz;
    float3 m = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y - y)).xyz;
    float3 downsampleColor = e * 0.125;
    downsampleColor += ((((a + c) + g) + i) * 0.03125);
    downsampleColor += ((((b + d) + f) + h) * 0.0625);
    downsampleColor += ((((j + k) + l) + m) * 0.125);
    out.downsample = float4(downsampleColor, 1.0);
    return out;
}
