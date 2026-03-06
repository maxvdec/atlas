#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

struct CameraUniforms {
    float4x4 invViewProj;
    float3 camPos;
    float _pad0;
};

struct Material {
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
    packed_float3 emissiveColor;
    float _pad0;
};

struct MeshData {
    uint vertexOffset;
    uint indexOffset;
    uint _pad0;
    uint _pad1;
};

struct InstanceData {
    float4x4 model;
    float4 normalCol0;
    float4 normalCol1;
    float4 normalCol2;
};

struct DirectionalLightData {
    float3 direction;
    float intensity;
    float3 color;
    float _pad0;
};

struct PointLight {
    packed_float3 position;
    float intensity;
    packed_float3 color;
    float range;
};

struct SpotLight {
    packed_float3 position;
    float intensity;

    packed_float3 direction;
    float innerCos;
    packed_float3 color;
    float outerCos;

    float range;
    float _pad0[3];
};

struct AreaLight {
    packed_float3 position;
    float intensity;

    packed_float3 right;
    float halfWidth;

    packed_float3 up;
    float halfHeight;

    packed_float3 color;
    float twoSided;
};

struct SceneData {
    uint numDirectionalLights;
    uint numPointLights;
    uint numSpotLights;
    uint numAreaLights;

    uint frameIndex;
    uint raysPerPixel;
    uint maxBounces;
    float indirectStrength;
};

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float luminance(float3 c) {
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float3 clampLuminance(float3 c, float maxL) {
    float l = luminance(c);
    if (l > maxL && l > 1e-6) {
        c *= maxL / l;
    }
    return c;
}

uint wang_hash(uint s) {
    s = (s ^ 61u) ^ (s >> 16u);
    s *= 9u;
    s = s ^ (s >> 4u);
    s *= 0x27d4eb2du;
    s = s ^ (s >> 15u);
    return s;
}

float rand(thread uint &state) {
    state = wang_hash(state);
    return float(state) / 4294967296.0;
}

uint seedBase(uint2 gid, uint w, uint frame, uint sampleIndex) {
    return gid.x + gid.y * w + frame * 9781u + sampleIndex * 6271u + 1u;
}

float3 skyColor(float3 dir, float intensity) { return float3(0, 0, 0); }

float3 cosineSampleHemisphere(float2 u) {
    float r = sqrt(u.x);
    float theta = 2.0 * M_PI_F * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u.x));

    return float3(x, y, z);
}

float3x3 buildOrthonormalBasis(float3 N) {
    float3 T = normalize(abs(N.x) > 0.1 ? cross(float3(0, 1, 0), N)
                                        : cross(float3(1, 0, 0), N));
    float3 B = cross(N, T);
    return float3x3(T, B, N);
}

float3 lambert(float3 albedo, float3 N, float3 L, float3 lightColor,
               float intensity) {
    float ndl = max(dot(N, L), 0.0);
    return albedo * lightColor * intensity * ndl;
}

bool isOccluded(intersector<triangle_data, instancing> isect,
                instance_acceleration_structure sceneAS, float3 P, float3 N,
                float3 L, float maxDistance) {
    ray shadowRay;
    shadowRay.origin = P + N * 0.001;
    shadowRay.direction = L;
    shadowRay.min_distance = 0.0;
    shadowRay.max_distance = maxDistance;

    auto shadowHit = isect.intersect(shadowRay, sceneAS, 0xFF);
    return shadowHit.type != intersection_type::none;
}

bool isOccludedDirectionalLight(DirectionalLightData light, float3 P, float3 N,
                                intersector<triangle_data, instancing> isect,
                                instance_acceleration_structure sceneAS) {
    float3 L = normalize(-light.direction);
    return isOccluded(isect, sceneAS, P, N, L, 1e30);
}

bool isOccludedPointLight(PointLight light, float3 P, float3 N,
                          intersector<triangle_data, instancing> isect,
                          instance_acceleration_structure sceneAS) {
    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 0.001);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

bool isOccludedSpotLight(SpotLight light, float3 P, float3 N,
                         intersector<triangle_data, instancing> isect,
                         instance_acceleration_structure sceneAS) {
    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

bool isOccludedAreaLight(AreaLight light, float3 P, float3 N,
                         intersector<triangle_data, instancing> isect,
                         instance_acceleration_structure sceneAS) {
    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

// ---------------------------------------------------------------------------
// PBR helpers: GGX / Cook-Torrance
// ---------------------------------------------------------------------------

float D_GGX(float NdotH, float roughness) {
    float a = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / max(M_PI_F * d * d, 1e-6);
}

float3 F_Schlick(float cosTheta, float3 F0) {
    float c = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow5(c);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

float disneyDiffuseFactor(float NdotV, float NdotL, float LdotH,
                          float roughness) {
    float fd90 = 0.5 + 2.0 * LdotH * LdotH * roughness;
    float lightScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotL);
    float viewScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotV);
    return lightScatter * viewScatter;
}

// GGX importance-sampled microfacet half-vector (in local TBN space, Z=up)
float3 sampleGGX(float2 u, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * M_PI_F * u.x;
    float cosTheta = sqrt((1.0 - u.y) / max(1.0 + (a * a - 1.0) * u.y, 1e-7));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Full Cook-Torrance PBR for a single analytic light
float3 evalPBR(float3 albedo, float metallic, float roughness, float3 N,
               float3 V, float3 L, float3 lightColor, float intensity) {
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float clampedRoughness = clamp(roughness, 0.045, 1.0);
    float3 F0 = mix(float3(0.04), albedo, clamp(metallic, 0.0, 1.0));
    float3 F = F_Schlick(VdotH, F0);
    float D = D_GGX(NdotH, clampedRoughness);
    float G = G_Smith(NdotV, NdotL, clampedRoughness);

    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
    float3 kD = (1.0 - F) * (1.0 - clamp(metallic, 0.0, 1.0));
    float diffuseFactor =
        disneyDiffuseFactor(NdotV, NdotL, max(dot(L, H), 0.0), clampedRoughness);
    float3 diffuse = (kD * albedo * diffuseFactor) / M_PI_F;

    return (diffuse + specular) * lightColor * intensity * NdotL;
}

float3 evalSubsurface(float3 albedo, float3 N, float3 V, float3 L,
                      float3 lightColor, float intensity, float roughness,
                      float sssStrength, float sssThickness) {
    float NdotL = dot(N, L);
    float NdotV = max(dot(N, V), 0.0);
    float backLit = clamp(-NdotL, 0.0, 1.0);
    float wrapAmount = mix(0.2, 0.7, clamp(roughness, 0.0, 1.0));
    float wrapped = clamp((NdotL + wrapAmount) / (1.0 + wrapAmount), 0.0, 1.0);
    float viewScatter =
        pow(clamp(1.0 - max(dot(V, L), 0.0), 0.0, 1.0), 2.0);
    float transmission = backLit * (0.35 + 0.65 * viewScatter);
    float diffuseBleed = wrapped * (0.5 + 0.5 * (1.0 - NdotV));
    float profile = mix(diffuseBleed, transmission, 0.65);
    float thicknessFalloff =
        exp(-max(sssThickness, 0.01) * (1.0 - backLit));
    return (albedo * lightColor * intensity * sssStrength * profile *
            thicknessFalloff) /
           M_PI_F;
}

// ---------------------------------------------------------------------------
// Direct lighting with full PBR (replaces old evalDirectLighting)
// ---------------------------------------------------------------------------

float3 evalDirectLightingPBR(intersector<triangle_data, instancing> isect,
                             instance_acceleration_structure sceneAS, float3 P,
                             float3 N, float3 V, float3 albedo, float metallic,
                             float roughness, float sssStrength,
                             float sssThickness,
                             constant DirectionalLightData &dirLight,
                             constant SceneData &sceneData,
                             constant PointLight *pointLights,
                             constant SpotLight *spotLights,
                             constant AreaLight *areaLights) {
    float3 lighting = float3(0.0);

    // Directional
    if (sceneData.numDirectionalLights > 0) {
        float3 L = normalize(-dirLight.direction);
        float3 c = evalPBR(albedo, metallic, roughness, N, V, L, dirLight.color,
                           max(dirLight.intensity, 0.0));
        float3 s = evalSubsurface(albedo, N, V, L, dirLight.color,
                                  max(dirLight.intensity, 0.0), roughness,
                                  sssStrength, sssThickness);
        if (!isOccludedDirectionalLight(dirLight, P, N, isect, sceneAS)) {
            float3 lightContribution = clampLuminance(c + s, 8.0);
            lighting += lightContribution;
        }
    }

    // Point lights
    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 toLight = pointLights[i].position - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float lightRange = max(pointLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(pointLights[i].intensity, 0.0) * atten;
        float3 c =
            evalPBR(albedo, metallic, roughness, N, V, L, pointLights[i].color,
                    intensity);
        float3 s = evalSubsurface(albedo, N, V, L, pointLights[i].color,
                                  intensity, roughness, sssStrength,
                                  sssThickness);
        if (!isOccludedPointLight(pointLights[i], P, N, isect, sceneAS)) {
            float3 lightContribution = clampLuminance(c + s, 8.0);
            lighting += lightContribution;
        }
    }

    // Spot lights
    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 toLight = spotLights[i].position - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 fwd = normalize(spotLights[i].direction);
        float spotCos = dot(-L, fwd);
        float spot =
            smoothstep(spotLights[i].outerCos, spotLights[i].innerCos, spotCos);
        float lightRange = max(spotLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(spotLights[i].intensity, 0.0) * atten * spot;
        float3 c =
            evalPBR(albedo, metallic, roughness, N, V, L, spotLights[i].color,
                    intensity);
        float3 s = evalSubsurface(albedo, N, V, L, spotLights[i].color,
                                  intensity, roughness, sssStrength,
                                  sssThickness);
        if (!isOccludedSpotLight(spotLights[i], P, N, isect, sceneAS)) {
            float3 lightContribution = clampLuminance(c + s, 8.0);
            lighting += lightContribution;
        }
    }

    // Area lights
    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float3 toLight = areaLights[i].position - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 lightNorm =
            normalize(cross(areaLights[i].right, areaLights[i].up));
        float cosLight = areaLights[i].twoSided > 0.5
                             ? abs(dot(lightNorm, -L))
                             : max(dot(lightNorm, -L), 0.0);
        float area = 4.0 * areaLights[i].halfWidth * areaLights[i].halfHeight;
        float minDist =
            max(max(areaLights[i].halfWidth, areaLights[i].halfHeight) * 0.5,
                0.15);
        float distSq = dist * dist + minDist * minDist;
        float atten = (cosLight * area) / max(distSq, 1e-4);
        float intensity = max(areaLights[i].intensity, 0.0) * atten;
        float3 c =
            evalPBR(albedo, metallic, roughness, N, V, L, areaLights[i].color,
                    intensity);
        float3 s = evalSubsurface(albedo, N, V, L, areaLights[i].color,
                                  intensity, roughness, sssStrength,
                                  sssThickness);
        if (!isOccludedAreaLight(areaLights[i], P, N, isect, sceneAS)) {
            float3 lightContribution = clampLuminance(c + s, 8.0);
            lighting += lightContribution;
        }
    }

    return lighting;
}

// ---------------------------------------------------------------------------
// sampleRadiance — primary path with GGX importance-sampled indirect bounce
// ---------------------------------------------------------------------------

float3 sampleRadiance(uint2 gid, uint sampleIndex, uint w,
                      intersector<triangle_data, instancing> isect,
                      instance_acceleration_structure sceneAS, ray primaryRay,
                      constant Material *materials, constant MeshData *meshData,
                      constant packed_float3 *vertices, constant uint *indices,
                      constant InstanceData *instanceData,
                      constant DirectionalLightData &dirLight,
                      constant SceneData &sceneData,
                      constant PointLight *pointLights,
                      constant SpotLight *spotLights,
                      constant AreaLight *areaLights) {
    uint rng = seedBase(gid, w, sceneData.frameIndex, sampleIndex);

    auto hit = isect.intersect(primaryRay, sceneAS, 0xFF);
    if (hit.type == intersection_type::none) {
        return skyColor(primaryRay.direction, 0.0);
    }

    uint instanceIndex = hit.instance_id;
    uint primitiveIndex = hit.primitive_id;

    Material mat = materials[instanceIndex];
    MeshData mesh = meshData[instanceIndex];
    InstanceData inst = instanceData[instanceIndex];

    uint i0 = indices[mesh.indexOffset + primitiveIndex * 3 + 0];
    uint i1 = indices[mesh.indexOffset + primitiveIndex * 3 + 1];
    uint i2 = indices[mesh.indexOffset + primitiveIndex * 3 + 2];

    float2 bary = hit.triangle_barycentric_coord;
    float b0 = 1.0 - bary.x - bary.y;
    float b1 = bary.x;
    float b2 = bary.y;

    float3 localN =
        normalize(float3(vertices[i0]) * b0 + float3(vertices[i1]) * b1 +
                  float3(vertices[i2]) * b2);

    float3x3 normalMatrix =
        float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
    float3 N = normalize(normalMatrix * localN);
    float3 P = primaryRay.origin + primaryRay.direction * hit.distance;
    float3 V = normalize(-primaryRay.direction);

    float roughness = clamp(mat.roughness, 0.08, 1.0);
    float metallic = clamp(mat.metallic, 0.0, 1.0);
    float ao = clamp(mat.ao, 0.0, 1.0);
    float emissiveIntensity = max(mat.emissiveIntensity, 0.0);
    float3 emissiveColor = float3(mat.emissiveColor);
    float3 emissive =
        clampLuminance(emissiveColor * min(emissiveIntensity, 8.0), 8.0);
    float sssStrength = clamp(1.0 - mat.albedo.w, 0.0, 1.0) * (1.0 - metallic);
    float sssThickness = mix(0.25, 1.75, ao);

    // --- Direct lighting (full PBR for all light types) ---
    float3 direct = evalDirectLightingPBR(
        isect, sceneAS, P, N, V, mat.albedo.xyz, metallic, roughness,
        sssStrength, sssThickness, dirLight, sceneData, pointLights, spotLights,
        areaLights);

    // --- Indirect bounce: GGX importance sampling ---
    float3 indirect = float3(0.0);

    if (sceneData.maxBounces > 0) {

        // Fresnel at normal incidence — drives specular vs diffuse split
        float3 F0 = mix(float3(0.04), mat.albedo.xyz, metallic);
        float3 F_approx = F_Schlick(max(dot(N, V), 0.0), F0);

        // Bias toward specular for metals / smooth surfaces
        float specProb =
            clamp(metallic * mix(0.35, 0.9, 1.0 - roughness), 0.0, 0.9);

        float3x3 basis = buildOrthonormalBasis(N);
        ray bounceRay;
        bounceRay.origin = P + N * 0.001;
        bounceRay.min_distance = 0.0;
        bounceRay.max_distance = 1.0e30;

        float3 brdfWeight;
        float chooseSplit = rand(rng);

        if (specProb > 1e-4 && chooseSplit < specProb) {
            // ---- Specular / reflective bounce (GGX importance sampled) ----
            float2 u = float2(rand(rng), rand(rng));
            float3 localH = sampleGGX(u, max(roughness, 0.001));
            float3 H_world = normalize(basis * localH);
            float3 bounceDir = reflect(-V, H_world);

            // Reject directions that go below the surface
            if (dot(bounceDir, N) <= 0.0)
                bounceDir = reflect(-V, N); // fallback: perfect mirror

            bounceRay.direction = bounceDir;

            float NdotL2 = max(dot(N, bounceDir), 1e-4);
            float NdotV2 = max(dot(N, V), 1e-4);
            float3 Fs = F_Schlick(max(dot(V, H_world), 0.0), F0);
            float Gs = G_Smith(NdotV2, NdotL2, roughness);

            // Weight = F*G / (4*NdotV) — PDF from GGX sampling cancels D term
            brdfWeight = (Fs * Gs / max(4.0 * NdotV2, 1e-4)) /
                         max(specProb, 1e-4);

        } else {
            // ---- Diffuse bounce (cosine-weighted hemisphere) ----
            float2 u = float2(rand(rng), rand(rng));
            float3 localBounce = cosineSampleHemisphere(u);
            bounceRay.direction = normalize(basis * localBounce);

            float3 kD = (1.0 - F_approx) * (1.0 - metallic);
            brdfWeight = (kD * mat.albedo.xyz) / max(1.0 - specProb, 1e-4);
        }

        brdfWeight = clamp(brdfWeight, float3(0.0), float3(1.25));

        auto bounceHit = isect.intersect(bounceRay, sceneAS, 0xFF);

        if (bounceHit.type == intersection_type::none) {
            // Hit the sky
            indirect = brdfWeight * skyColor(bounceRay.direction, 0.0) *
                       sceneData.indirectStrength;
        } else {
            // --- Shade the bounce surface with full PBR direct lighting ---
            uint bi = bounceHit.instance_id;
            uint bp = bounceHit.primitive_id;

            Material bmat = materials[bi];
            MeshData bmesh = meshData[bi];
            InstanceData binst = instanceData[bi];

            uint bj0 = indices[bmesh.indexOffset + bp * 3 + 0];
            uint bj1 = indices[bmesh.indexOffset + bp * 3 + 1];
            uint bj2 = indices[bmesh.indexOffset + bp * 3 + 2];

            float2 bbary = bounceHit.triangle_barycentric_coord;
            float bb0 = 1.0 - bbary.x - bbary.y;

            float3 blocN = normalize(float3(vertices[bj0]) * bb0 +
                                     float3(vertices[bj1]) * bbary.x +
                                     float3(vertices[bj2]) * bbary.y);

            float3x3 bNM = float3x3(binst.normalCol0.xyz, binst.normalCol1.xyz,
                                    binst.normalCol2.xyz);
            float3 bN = normalize(bNM * blocN);
            float3 bP =
                bounceRay.origin + bounceRay.direction * bounceHit.distance;
            float3 bV = -bounceRay.direction;

            float bRoughness = clamp(bmat.roughness, 0.08, 1.0);
            float bMetallic = clamp(bmat.metallic, 0.0, 1.0);
            float bAo = clamp(bmat.ao, 0.0, 1.0);
            float bEmissiveIntensity = clamp(bmat.emissiveIntensity, 0.0, 8.0);
            float3 bEmissiveColor = float3(bmat.emissiveColor);
            float3 bEmissive =
                clampLuminance(bEmissiveColor * bEmissiveIntensity, 8.0);
            float bSssStrength =
                clamp(1.0 - bmat.albedo.w, 0.0, 1.0) * (1.0 - bMetallic);
            float bSssThickness = mix(0.25, 1.75, bAo);

            float3 bounceDirect = evalDirectLightingPBR(
                isect, sceneAS, bP, bN, bV, bmat.albedo.xyz, bMetallic,
                bRoughness, bSssStrength, bSssThickness, dirLight, sceneData,
                pointLights, spotLights, areaLights);

            float3 bAmbient = bmat.albedo.xyz * 0.04 * (1.0 - bMetallic) * bAo;
            indirect = brdfWeight * (bAmbient + bounceDirect + bEmissive) *
                       sceneData.indirectStrength;
        }

        indirect = clampLuminance(indirect, 3.0);
    }

    float3 ambient = mat.albedo.xyz * 0.04 * (1.0 - metallic) * ao;
    return clampLuminance(ambient + direct + emissive + indirect, 10.0);
}

kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  texture2d<float, access::read> prevTex [[texture(1)]],
                  instance_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant MeshData *meshData [[buffer(3)]],
                  constant packed_float3 *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
                  constant SceneData &sceneData [[buffer(8)]],
                  constant PointLight *pointLights [[buffer(9)]],
                  constant SpotLight *spotLights [[buffer(10)]],
                  constant AreaLight *areaLights [[buffer(11)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint w = outTex.get_width();
    uint h = outTex.get_height();
    if (gid.x >= w || gid.y >= h)
        return;

    float2 uv = (float2(gid) + 0.5) / float2(w, h);
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;

    float4 clip = float4(ndc, 1.0, 1.0);
    float4 worldH = cam.invViewProj * clip;
    float3 worldP = worldH.xyz / worldH.w;

    float3 ro = cam.camPos;
    float3 rd = normalize(worldP - ro);

    intersector<triangle_data, instancing> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.set_triangle_cull_mode(triangle_cull_mode::none);

    ray r;
    r.origin = ro;
    r.direction = rd;
    r.min_distance = 0.001;
    r.max_distance = 1.0e30;

    auto hit = isect.intersect(r, sceneAS, 0xFF);

    if (hit.type == intersection_type::none) {
        outTex.write(float4(0, 0, 0, 1), gid);
    } else {
        float3 color = float3(0.0);

        uint spp = max(sceneData.raysPerPixel, 1u);
        for (uint s = 0; s < spp; ++s) {
            ray primaryRay;
            primaryRay.origin = ro;
            primaryRay.direction = rd;
            primaryRay.min_distance = 0.001;
            primaryRay.max_distance = 1.0e30;

            float3 sample = sampleRadiance(gid, s, w, isect, sceneAS, primaryRay,
                                           materials, meshData, vertices, indices,
                                           instanceData, dirLight, sceneData,
                                           pointLights, spotLights, areaLights);
            color += clampLuminance(sample, 10.0);
        }

        color /= float(spp);

        int frameIndex = int(sceneData.frameIndex);

        float4 prevColor = prevTex.read(gid);
        if (frameIndex == 0)
            prevColor = float4(0, 0, 0, 1);

        if (frameIndex > 2) {
            float prevL = luminance(prevColor.xyz);
            float currL = luminance(color);
            float maxAllowed = max(prevL * 1.6 + 0.15, 0.75);
            if (currL > maxAllowed && currL > 1e-6) {
                color *= maxAllowed / currL;
            }
        }

        float3 accum = (prevColor.xyz * frameIndex + color) / (frameIndex + 1);
        accum = clampLuminance(accum, 10.0);
        outTex.write(float4(accum, 1.0), gid);
    }
}
