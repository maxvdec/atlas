#include <metal_stdlib>
using namespace metal;

struct ProbeSpace {
    float3 origin;
    float _pad0;

    float3 spacing;
    float _pad1;

    float3 probeCount;
    float _pad2;

    float4 debugColor;

    float4 atlasParams;
    // x = textureBorderSize
    // y = probeResolution (inner)
    // z = probesPerRow
    // w = totalProbes  (put this here on CPU!)
};

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]],
                  constant ProbeSpace &ps [[buffer(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint border = ps.atlasParams.x;
    uint innerRes = ps.atlasParams.y;
    uint tilesPerRow = ps.atlasParams.z;
    uint totalProbes = ps.atlasParams.w;

    uint tileRes = innerRes + 2u * border;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height())
        return;

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;

    uint probeIndex = tileX + tileY * tilesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(
            float4(float(0.02), float(0.02), float(0.02), float(1.0)), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    bool isOutline = (localX == 0u) || (localY == 0u) ||
                     (localX == tileRes - 1u) || (localY == tileRes - 1u);

    float4 outlineColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 fillColor = float4(ps.debugColor.xyz, 1.0);

    outTexture.write(isOutline ? outlineColor : fillColor, gid);
}
