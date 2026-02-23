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
    float2 texelSize = float2(1.0) / _13.srcResolution;
    float x = _13.filterRadius * texelSize.x;
    float y = _13.filterRadius * texelSize.y;
    float2 texCoord = in.TexCoord;
    float3 a = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y + y)).xyz;
    float3 b = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y + y)).xyz;
    float3 c = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y + y)).xyz;
    float3 d = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y)).xyz;
    float3 e = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y)).xyz;
    float3 f = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y)).xyz;
    float3 g = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y - y)).xyz;
    float3 h = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y - y)).xyz;
    float3 i = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y - y)).xyz;
    float3 upsampleColor = e * 4.0;
    upsampleColor += ((((b + d) + f) + h) * 2.0);
    upsampleColor += (((a + c) + g) + i);
    upsampleColor *= 0.0625;
    out.upsample = float4(upsampleColor, 1.0);
    return out;
}
