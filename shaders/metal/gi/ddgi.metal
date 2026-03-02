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

static inline float3 octDecode(float2 e) {
    float3 n = float3(e.x, e.y, 1.0f - fabs(e.x) - fabs(e.y));

    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(n.x >= 0.0f ? 1.0f : -1.0f, n.y >= 0.0f ? 1.0f : -1.0f);
        float2 folded = (1.0f - fabs(n.yx)) * signNotZero;
        n.x = folded.x;
        n.y = folded.y;
    }

    return normalize(n);
}

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]],
                  constant ProbeSpace &ps [[buffer(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint border = ps.atlasParams.x;
    uint innerRes = ps.atlasParams.y;
    uint tilesPerRow = ps.atlasParams.z;
    uint totalProbes = ps.atlasParams.w;

    uint tileRes = innerRes + 2u * border;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height()) {
        return;
    }

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;
    uint probeIndex = tileX + tileY * tilesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(float4(float3(0.0), 1.0), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    bool isOutline = (localX == 0u) || (localY == 0u) ||
                     (localX == tileRes - 1u) || (localY == tileRes - 1u);

    bool isInner = (localX >= border) && (localX < border + innerRes) &&
                   (localY >= border) && (localY < border + innerRes);

    if (isOutline) {
        outTexture.write(float4(1, 1, 1, 1), gid);
        return;
    }

    if (!isInner) {
        outTexture.write(float4(0.05f, 0.05f, 0.05f, 1.0f), gid);
    }

    float2 uv =
        (float2(localX - border, localY - border) + 0.5f) / float(innerRes);
    float2 e = uv * 2.0 - 1.0;

    float3 dir = octDecode(e);

    float3 rgb = dir * 0.5 + 0.5;

    outTexture.write(float4(rgb, 1.0), gid);
}
