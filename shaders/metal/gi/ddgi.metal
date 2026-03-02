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

static inline bool rayTriangleMT(float3 ro, float3 rd, float3 v0, float3 v1,
                                 float3 v2, thread float &t, thread float &u,
                                 thread float &v) {
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 p = cross(rd, e2);
    float det = dot(e1, p);

    if (fabs(det) < 1e-8f)
        return false;
    float invDet = 1.0f / det;

    float3 s = ro - v0;
    u = dot(s, p) * invDet;
    if (u < 0.0f, || u > 1.0f)
        return false;

    float3 q = cross(s, e1);
    v = dot(rd, q) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = dot(e2, q) * invDet;
    return t > 0.0f;
}

static inline Hit traceScene(float3 ro, float3 rd, device const Triangle *tris,
                             uint triCount) {
    Hit best;
    best.t = INFINITY;
    best.hit = false;
    best.materialID = -1;
    best.n = float3(0.0f, 1.0f, 0.0f);

    for (uint i = 0; i < triCount; i++) {
        float t, u, v;
        if (rayTriangleMT(ro, rd, tris[i].v0, tris[i].v1, tris[i].v2, t, u,
                          v)) {
            if (t < best.t) {
                best.t = t;
                float w = 1.0f - u - v;
                float3 n =
                    normalize(tris[i].n0 * w + tris[i].n1 * u + tris[i].n2 * v);
                best.n = n;
                best.materialID = tris[i].materialID;
                best.hit = true;
            }
        }
    }

    return best;
}

static inline uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

static inline float rand01(thread uint &state) {
    state = wangHash(state);
    return (float)(state & 0x00FFFFFFu) / 16777216.0f;
}

static inline float3 randomUnitVector(thread uint &state) {
    float z = rand01(state) * 2.0f - 1.0f;
    float a = rand01(state) * 6.28318530718f;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    return float3(r * cos(a), z, r * sin(a));
}

static inline uint3 probeCoordFromIndex(uint idx, uint3 counts) {
    uint xy = counts.x * counts.y;
    uint z = idx / xy;
    uint rem = idx - z * xy;
    uint y = rem / counts.x;
    uint x = rem - y * counts.x;
    return uint3(x, y, z);
}

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

kernel void main0(device float3 *probeRadianceOut [[buffer(0)]],
                  device const Triangle *tris [[buffer(1)]],
                  constant uint &triCount [[buffer(2)]],
                  constant ProbeSpace &ps [[buffer(3)]],
                  constant RaytracingSettings &rt [[buffer(4)]],
                  uint tid [[thread_position_in_grid]]) {
    uint totalProbes = (uint)ps.atlasParams.w;
    if (tid >= totalProbes)
        return;

    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);
    uint3 pc = probeCoordFromIndex(tid, counts);

    float3 probePos = ps.origin + float3(pc) * ps.spacing;

    uint state = tid * 9781u + rt.frameIndex * 6271u;

    float3 sum = float3(0.0f);

    for (uint r = 0; r < rt.raysPerProbe; r++) {
        float3 rd = randomUnitVector(state);
        float3 ro = probePos + rd * rt.normalBias;

        Hit h = traceScene(ro, rd, tris, triCount);

        float3 radiance;
        if (!h.hit) {
            radiance = float3(0.1f, 0.15f, 0.25f);
        } else {
            float3 L = normalize(float3(0.5, 1.0, 0.5));
            float ndl = max(0.0f, dot(h.n, L));
            radiance = float3(1.0, 0.95, 0.8) * ndl;
        }

        sum += radiance;
    }

    probeRadianceOut[tid] = sum / float(rt.raysPerProbe);
}
