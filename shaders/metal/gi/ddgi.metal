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

struct Material {
    int materialID;
    float metallic;
    float roughness;
    float ao;

    packed_float3 albedo;
    float _pad0;
};

struct Triangle {
    float4 v0;
    float4 v1;
    float4 v2;
    float4 n0;
    float4 n1;
    float4 n2;
    int materialID;
    int padding[3];
};

struct SceneCounts {
    uint triCount;
    uint materialCount;
    uint directionalLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint areaLightCount;
    uint _pad0;
    uint _pad1;
};

struct DirectionalLight {
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct PointLight {
    packed_float3 position;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct SpotLight {
    packed_float3 position;
    float _pad1;
    packed_float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    packed_float3 diffuse;
    float _pad5;
    packed_float3 specular;
    float _pad6;
};

struct AreaLight {
    packed_float3 position;
    float _pad1;
    packed_float3 right;
    float _pad2;
    packed_float3 up;
    float _pad3;
    float2 size;
    float _pad4;
    float _pad5;
    packed_float3 diffuse;
    float _pad6;
    packed_float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
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
    if (u < 0.0f || u > 1.0f)
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
        if (rayTriangleMT(ro, rd, tris[i].v0.xyz, tris[i].v1.xyz,
                          tris[i].v2.xyz, t, u, v)) {
            if (t < best.t) {
                best.t = t;
                float w = 1.0f - u - v;
                float3 n = normalize(tris[i].n0.xyz * w + tris[i].n1.xyz * u +
                                     tris[i].n2.xyz * v);
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

static inline uint3 probeCoordFromIndex(uint idx, uint3 counts) {
    uint xy = counts.x * counts.y;
    uint z = idx / xy;
    uint rem = idx - z * xy;
    uint y = rem / counts.x;
    uint x = rem - y * counts.x;
    return uint3(x, y, z);
}

static inline float3 safeNormalize(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10f) {
        return v * rsqrt(len2);
    }
    return fallback;
}

static inline float3 sampleSky(float3 d) {
    float t = clamp(d.y * 0.5f + 0.5f, 0.0f, 1.0f);
    return mix(float3(0.0002f, 0.0002f, 0.0002f),
               float3(0.0012f, 0.0011f, 0.0010f),
               t);
}

static inline float shadowVisibility(float3 ro, float3 rd, float maxT,
                                     device const Triangle *tris,
                                     uint triCount) {
    if (maxT <= 1e-4f) {
        return 1.0f;
    }
    Hit h = traceScene(ro, rd, tris, triCount);
    if (!h.hit) {
        return 1.0f;
    }
    float minOccluderDistance = max(0.01f, maxT * 0.0025f);
    if (h.t <= minOccluderDistance) {
        return 1.0f;
    }
    return (h.t >= maxT) ? 1.0f : 0.0f;
}

static inline float giNdotL(float3 n, float3 l) {
    return max(dot(n, l), 0.0f);
}

static inline float3 evaluateDirectLights(
    float3 posWS, float3 normalWS, float bias, float maxDistance,
    device const Triangle *tris, uint triCount,
    device const DirectionalLight *directionalLights, uint directionalLightCount,
    device const PointLight *pointLights, uint pointLightCount,
    device const SpotLight *spotLights, uint spotLightCount,
    device const AreaLight *areaLights, uint areaLightCount) {
    float3 n = safeNormalize(normalWS, float3(0.0f, 1.0f, 0.0f));
    float3 sum = float3(0.0f);

    for (uint i = 0; i < directionalLightCount; i++) {
        float3 L = safeNormalize(-directionalLights[i].direction,
                                 float3(0.0f, 1.0f, 0.0f));
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        sum += directionalLights[i].diffuse *
               max(0.0f, directionalLights[i].intensity) * ndl;
    }

    for (uint i = 0; i < pointLightCount; i++) {
        float3 toLight = pointLights[i].position - posWS;
        float dist = length(toLight);
        float radius = max(pointLights[i].radius, 0.001f);
        if (dist <= 1e-4f || dist >= radius)
            continue;

        float3 L = toLight / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float attenuation =
            1.0f / max(pointLights[i].constant0 + pointLights[i].linear * dist +
                           pointLights[i].quadratic * dist * dist,
                       1e-4f);
        float fade = 1.0f - smoothstep(radius * 0.9f, radius, dist);
        sum += pointLights[i].diffuse * max(0.0f, pointLights[i].intensity) *
               attenuation * fade * ndl;
    }

    for (uint i = 0; i < spotLightCount; i++) {
        float3 toLight = spotLights[i].position - posWS;
        float dist = length(toLight);
        float range = max(spotLights[i].range, 0.001f);
        if (dist <= 1e-4f || dist >= range)
            continue;

        float3 L = toLight / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float3 spotDir = safeNormalize(spotLights[i].direction,
                                       float3(0.0f, -1.0f, 0.0f));
        float theta = dot(L, -spotDir);
        float epsilon = max(spotLights[i].cutOff - spotLights[i].outerCutOff,
                            1e-4f);
        float cone = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0f,
                           1.0f);
        if (cone <= 0.0f)
            continue;

        float attenuation =
            1.0f / ((1.0f + (dist / range)) + ((dist * dist) / (range * range)));
        float fade = 1.0f - smoothstep(range * 0.9f, range, dist);
        sum += spotLights[i].diffuse * max(0.0f, spotLights[i].intensity) * cone *
               attenuation * fade * ndl;
    }

    for (uint i = 0; i < areaLightCount; i++) {
        float3 center = areaLights[i].position;
        float3 right = safeNormalize(areaLights[i].right, float3(1.0f, 0.0f, 0.0f));
        float3 up = safeNormalize(areaLights[i].up, float3(0.0f, 1.0f, 0.0f));
        float2 halfSize = areaLights[i].size * 0.5f;

        float3 toPoint = posWS - center;
        float s = clamp(dot(toPoint, right), -halfSize.x, halfSize.x);
        float t = clamp(dot(toPoint, up), -halfSize.y, halfSize.y);
        float3 closest = center + right * s + up * t;

        float3 Lvec = closest - posWS;
        float dist = length(Lvec);
        float range = max(areaLights[i].range, 0.001f);
        if (dist <= 1e-4f || dist >= range)
            continue;

        float3 L = Lvec / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float3 lightNormal =
            safeNormalize(cross(right, up), float3(0.0f, -1.0f, 0.0f));
        float nl = dot(lightNormal, -L);
        float facing = (areaLights[i].castsBothSides != 0) ? abs(nl) : max(nl, 0.0f);
        float cutoff = cos(areaLights[i].angle * 0.01745329251f);
        if (facing < cutoff || facing <= 0.0f)
            continue;

        float attenuation =
            1.0f / ((1.0f + (dist / range)) + ((dist * dist) / (range * range)));
        float fade = 1.0f - smoothstep(range * 0.9f, range, dist);
        sum += areaLights[i].diffuse * max(0.0f, areaLights[i].intensity) *
               facing * attenuation * fade * ndl;
    }

    return sum;
}

static inline void buildBasis(float3 n, thread float3 &t, thread float3 &b) {
    float3 up = (fabs(n.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f)
                                     : float3(1.0f, 0.0f, 0.0f);
    t = safeNormalize(cross(up, n), float3(1.0f, 0.0f, 0.0f));
    b = safeNormalize(cross(n, t), float3(0.0f, 0.0f, 1.0f));
}

static inline float3 sampleCosineHemisphere(thread uint &state) {
    float u1 = rand01(state);
    float u2 = rand01(state);
    float r = sqrt(max(0.0f, u1));
    float phi = 6.28318530718f * u2;
    float x = r * cos(phi);
    float z = r * sin(phi);
    float y = sqrt(max(0.0f, 1.0f - u1));
    return float3(x, y, z);
}

static inline float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    const float GOLDEN_RATIO = 1.6180339887498949f;
    const float TAU = 6.28318530718f;
    float i = float(index) + 0.5f;
    float phi = TAU * fract(i / GOLDEN_RATIO);
    float cosTheta = 1.0f - (2.0f * i) / float(count);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    uint rotSeed = wangHash(frameIndex * 1471u + 5743u);
    float rotAngle = float(rotSeed & 0xFFFFu) / 65536.0f * TAU;
    phi += rotAngle;
    return float3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));
}

kernel void main0(device float4 *probeRadianceOut [[buffer(0)]],
                  device const Triangle *tris [[buffer(1)]],
                  device const Material *materials [[buffer(2)]],
                  constant SceneCounts &sc [[buffer(3)]],
                  constant ProbeSpace &ps [[buffer(4)]],
                  constant RaytracingSettings &rt [[buffer(5)]],
                  device const DirectionalLight *directionalLights [[buffer(6)]],
                  device const PointLight *pointLights [[buffer(7)]],
                  device const SpotLight *spotLights [[buffer(8)]],
                  device const AreaLight *areaLights [[buffer(9)]],
                  uint tid [[thread_position_in_grid]]) {
    const float PI = 3.14159265359f;
    uint totalProbes = (uint)ps.atlasParams.w;
    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    uint totalRays = totalProbes * raysPerProbe;

    if (tid >= totalRays)
        return;

    uint probeIndex = tid / raysPerProbe;
    uint rayIndex = tid - probeIndex * raysPerProbe;

    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);
    uint3 pc = probeCoordFromIndex(probeIndex, counts);
    float3 probePos = ps.origin + float3(pc) * ps.spacing;

    float3 rayDir = sphericalFibonacci(rayIndex, raysPerProbe, rt.frameIndex);

    float maxDistance = max(rt.maxRayDistance, 0.001f);
    float bias = max(rt.normalBias, 0.001f);

    float3 ro = probePos + rayDir * bias;
    Hit h;
    float selfHitThreshold = bias * 4.0f;
    for (uint escapeStep = 0u; escapeStep < 1u; escapeStep++) {
        h = traceScene(ro, rayDir, tris, sc.triCount);
        if (!h.hit || h.t >= selfHitThreshold) {
            break;
        }
        ro += rayDir * (h.t + selfHitThreshold);
    }
    if (h.hit && h.t < selfHitThreshold) {
        h.hit = false;
        h.t = INFINITY;
    }

    float3 radiance = float3(0.0f);
    if (!h.hit || h.t > maxDistance) {
        radiance = sampleSky(rayDir);
    } else {
        float3 hitPos = ro + rayDir * h.t;
        float3 hitNormal = safeNormalize(h.n, float3(0.0f, 1.0f, 0.0f));

        if (dot(hitNormal, -rayDir) < 0.0f) {
            hitNormal = -hitNormal;
        }

        float3 albedo = float3(0.7f);
        if (h.materialID >= 0 && (uint)h.materialID < sc.materialCount) {
            albedo = clamp(materials[h.materialID].albedo, float3(0.0f),
                           float3(1.0f));
        }

        float3 direct = evaluateDirectLights(
            hitPos, hitNormal, bias, maxDistance, tris, sc.triCount,
            directionalLights, sc.directionalLightCount, pointLights,
            sc.pointLightCount, spotLights, sc.spotLightCount, areaLights,
            sc.areaLightCount);

        radiance = direct * albedo * 0.6f;

    }

    probeRadianceOut[tid] = float4(radiance, h.hit ? h.t : -1.0f);
}
