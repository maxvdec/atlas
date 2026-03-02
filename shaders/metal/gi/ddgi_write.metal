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

struct Material {
    int materialID;
    float metallic;
    float roughness;
    float ao;

    float3 albedo;
    float _pad0;
};

struct Triangle {
    float3 v0;
    float3 v1;
    float3 v2;

    float3 n0;
    float3 n1;
    float3 n2;

    int materialID;
};

struct Hit {
    float t;
    float3 n;
    int materialID;
    bool hit;
};

kernel void main0(texture2d<float4, access::write> outTexture [[texture(0)]],
                  texture2d<float4, access::read> prevTexture [[texture(1)]],
                  device const float3 *probeRadiance [[buffer(0)]],
                  constant ProbeSpace &ps [[buffer(1)]],
                  constant RaytracingSettings &rt [[buffer(2)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    uint totalProbes = (uint)ps.atlasParams.w;

    uint tileRes = innerRes + 2u * border;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height())
        return;

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;
    uint probeIndex = tileX + tileY * probesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(float4(0, 0, 0, 1), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    bool isInner = (localX >= border) && (localX < border + innerRes) &&
                   (localY >= border) && (localY < border + innerRes);

    if (!isInner) {
        outTexture.write(float4(0.02f, 0.02f, 0.02f, 1), gid);
        return;
    }

    float3 cur = probeRadiance[probeIndex];

    float4 prev = prevTexture.read(gid);
    float h = rt.hysteresis;
    float3 blended = mix(cur, prev.xyz, h);

    outTexture.write(float4(blended, 1.0f), gid);
}
