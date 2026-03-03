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
};

struct RaytracingSettings {
    uint raysPerProbe;
    float maxRayDistance;
    float normalBias;
    float hysteresis;

    uint frameIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]],
                  texture2d<float, access::read> prevTexture [[texture(1)]],
                  device float4 *probeRadiance [[buffer(0)]],
                  constant ProbeSpace &ps [[buffer(1)]],
                  constant RaytracingSettings &rt [[buffer(2)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    uint totalProbes = (uint)ps.atlasParams.w;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height())
        return;

    uint tileRes = innerRes + 2u * border;
    if (tileRes == 0u || probesPerRow == 0u || totalProbes == 0u ||
        innerRes == 0u) {
        outTexture.write(prevTexture.read(gid), gid);
        return;
    }

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;
    uint probeIndex = tileX + tileY * probesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(prevTexture.read(gid), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    int innerX = clamp(int(localX) - int(border), 0, int(innerRes) - 1);
    int innerY = clamp(int(localY) - int(border), 0, int(innerRes) - 1);

    uint texelsPerProbe = innerRes * innerRes;
    uint probeTexelIndex = probeIndex * texelsPerProbe +
                           (uint(innerY) * innerRes + uint(innerX));

    float3 cur = probeRadiance[probeTexelIndex].xyz;
    if (!all(isfinite(cur))) {
        cur = float3(0.0f);
    }

    float4 prev = prevTexture.read(gid);
    float3 prevValue = all(isfinite(prev.xyz)) ? prev.xyz : float3(0.0f);
    float h = clamp(rt.hysteresis, 0.0f, 0.995f);
    float3 blended = (rt.frameIndex == 0u) ? cur : mix(cur, prevValue, h);

    outTexture.write(float4(max(blended, float3(0.0f)), 1.0f), gid);
}
