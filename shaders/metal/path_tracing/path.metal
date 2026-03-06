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
    int albedoTextureIndex;
    int normalTextureIndex;
    int metallicTextureIndex;
    int roughnessTextureIndex;
    int aoTextureIndex;
    int _pad1[3];
};

struct MeshData {
    uint vertexOffset;
    uint indexOffset;
    uint _pad0;
    uint _pad1;
};

struct VertexData {
    packed_float3 normal;
    packed_float2 uv;
    packed_float3 tangent;
    packed_float3 bitangent;
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
    uint materialTextureCount;
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

float3 normalizeOr(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10) {
        return v * rsqrt(len2);
    }
    return fallback;
}

constexpr sampler materialTexSampler(coord::normalized, address::repeat,
                                     filter::linear, mip_filter::linear);

float4 sampleMaterialTexture(int textureIndex, float2 uv,
                             texture2d<float> materialTexture0,
                             texture2d<float> materialTexture1,
                             texture2d<float> materialTexture2,
                             texture2d<float> materialTexture3,
                             texture2d<float> materialTexture4,
                             texture2d<float> materialTexture5,
                             texture2d<float> materialTexture6,
                             texture2d<float> materialTexture7,
                             texture2d<float> materialTexture8,
                             texture2d<float> materialTexture9,
                             texture2d<float> materialTexture10,
                             texture2d<float> materialTexture11,
                             texture2d<float> materialTexture12,
                             texture2d<float> materialTexture13,
                             texture2d<float> materialTexture14,
                             texture2d<float> materialTexture15,
                             texture2d<float> materialTexture16,
                             texture2d<float> materialTexture17,
                             texture2d<float> materialTexture18,
                             texture2d<float> materialTexture19) {
    switch (textureIndex) {
    case 0:
        return materialTexture0.sample(materialTexSampler, uv);
    case 1:
        return materialTexture1.sample(materialTexSampler, uv);
    case 2:
        return materialTexture2.sample(materialTexSampler, uv);
    case 3:
        return materialTexture3.sample(materialTexSampler, uv);
    case 4:
        return materialTexture4.sample(materialTexSampler, uv);
    case 5:
        return materialTexture5.sample(materialTexSampler, uv);
    case 6:
        return materialTexture6.sample(materialTexSampler, uv);
    case 7:
        return materialTexture7.sample(materialTexSampler, uv);
    case 8:
        return materialTexture8.sample(materialTexSampler, uv);
    case 9:
        return materialTexture9.sample(materialTexSampler, uv);
    case 10:
        return materialTexture10.sample(materialTexSampler, uv);
    case 11:
        return materialTexture11.sample(materialTexSampler, uv);
    case 12:
        return materialTexture12.sample(materialTexSampler, uv);
    case 13:
        return materialTexture13.sample(materialTexSampler, uv);
    case 14:
        return materialTexture14.sample(materialTexSampler, uv);
    case 15:
        return materialTexture15.sample(materialTexSampler, uv);
    case 16:
        return materialTexture16.sample(materialTexSampler, uv);
    case 17:
        return materialTexture17.sample(materialTexSampler, uv);
    case 18:
        return materialTexture18.sample(materialTexSampler, uv);
    case 19:
        return materialTexture19.sample(materialTexSampler, uv);
    default:
        break;
    }
    return float4(0.0);
}

void resolveMaterialParameters(Material mat, float2 uv, uint textureCount,
                               texture2d<float> materialTexture0,
                               texture2d<float> materialTexture1,
                               texture2d<float> materialTexture2,
                               texture2d<float> materialTexture3,
                               texture2d<float> materialTexture4,
                               texture2d<float> materialTexture5,
                               texture2d<float> materialTexture6,
                               texture2d<float> materialTexture7,
                               texture2d<float> materialTexture8,
                               texture2d<float> materialTexture9,
                               texture2d<float> materialTexture10,
                               texture2d<float> materialTexture11,
                               texture2d<float> materialTexture12,
                               texture2d<float> materialTexture13,
                               texture2d<float> materialTexture14,
                               texture2d<float> materialTexture15,
                               texture2d<float> materialTexture16,
                               texture2d<float> materialTexture17,
                               texture2d<float> materialTexture18,
                               texture2d<float> materialTexture19,
                               thread float3 &albedo, thread float &metallic,
                               thread float &roughness, thread float &ao,
                               thread float3 &emissive) {
    albedo = mat.albedo.xyz;
    metallic = mat.metallic;
    roughness = mat.roughness;
    ao = mat.ao;
    emissive =
        clampLuminance(float3(mat.emissiveColor) * min(max(mat.emissiveIntensity, 0.0), 8.0),
                       8.0);

    if (mat.albedoTextureIndex >= 0 &&
        uint(mat.albedoTextureIndex) < textureCount) {
        float3 tex = sampleMaterialTexture(
                         mat.albedoTextureIndex, uv, materialTexture0,
                         materialTexture1, materialTexture2, materialTexture3,
                         materialTexture4, materialTexture5, materialTexture6,
                         materialTexture7, materialTexture8, materialTexture9,
                         materialTexture10, materialTexture11,
                         materialTexture12, materialTexture13,
                         materialTexture14, materialTexture15,
                         materialTexture16, materialTexture17,
                         materialTexture18, materialTexture19)
                         .xyz;
        albedo *= clamp(tex, float3(0.0), float3(1.0));
    }
    if (mat.metallicTextureIndex >= 0 &&
        uint(mat.metallicTextureIndex) < textureCount) {
        metallic *= clamp(sampleMaterialTexture(
                              mat.metallicTextureIndex, uv, materialTexture0,
                              materialTexture1, materialTexture2,
                              materialTexture3, materialTexture4,
                              materialTexture5, materialTexture6,
                              materialTexture7, materialTexture8,
                              materialTexture9, materialTexture10,
                              materialTexture11, materialTexture12,
                              materialTexture13, materialTexture14,
                              materialTexture15, materialTexture16,
                              materialTexture17, materialTexture18,
                              materialTexture19)
                              .x,
                          0.0, 1.0);
    }
    if (mat.roughnessTextureIndex >= 0 &&
        uint(mat.roughnessTextureIndex) < textureCount) {
        roughness *= clamp(sampleMaterialTexture(
                               mat.roughnessTextureIndex, uv, materialTexture0,
                               materialTexture1, materialTexture2,
                               materialTexture3, materialTexture4,
                               materialTexture5, materialTexture6,
                               materialTexture7, materialTexture8,
                               materialTexture9, materialTexture10,
                               materialTexture11, materialTexture12,
                               materialTexture13, materialTexture14,
                               materialTexture15, materialTexture16,
                               materialTexture17, materialTexture18,
                               materialTexture19)
                               .x,
                           0.0, 1.0);
    }
    if (mat.aoTextureIndex >= 0 && uint(mat.aoTextureIndex) < textureCount) {
        ao *= clamp(sampleMaterialTexture(
                        mat.aoTextureIndex, uv, materialTexture0,
                        materialTexture1, materialTexture2, materialTexture3,
                        materialTexture4, materialTexture5, materialTexture6,
                        materialTexture7, materialTexture8, materialTexture9,
                        materialTexture10, materialTexture11,
                        materialTexture12, materialTexture13,
                        materialTexture14, materialTexture15,
                        materialTexture16, materialTexture17,
                        materialTexture18, materialTexture19)
                        .x,
                    0.0, 1.0);
    }

    albedo = clamp(albedo, float3(0.0), float3(1.0));
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.08, 1.0);
    ao = clamp(ao, 0.0, 1.0);
}

float3 resolveShadingNormal(Material mat, float2 uv, float3 localN,
                            float3 localT, float3 localB,
                            InstanceData inst, uint textureCount,
                            texture2d<float> materialTexture0,
                            texture2d<float> materialTexture1,
                            texture2d<float> materialTexture2,
                            texture2d<float> materialTexture3,
                            texture2d<float> materialTexture4,
                            texture2d<float> materialTexture5,
                            texture2d<float> materialTexture6,
                            texture2d<float> materialTexture7,
                            texture2d<float> materialTexture8,
                            texture2d<float> materialTexture9,
                            texture2d<float> materialTexture10,
                            texture2d<float> materialTexture11,
                            texture2d<float> materialTexture12,
                            texture2d<float> materialTexture13,
                            texture2d<float> materialTexture14,
                            texture2d<float> materialTexture15,
                            texture2d<float> materialTexture16,
                            texture2d<float> materialTexture17,
                            texture2d<float> materialTexture18,
                            texture2d<float> materialTexture19) {
    float3x3 normalMatrix =
        float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
    float3 N = normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0));

    float3 T = normalizeOr((inst.model * float4(localT, 0.0)).xyz,
                           float3(1.0, 0.0, 0.0));
    float3 B = normalizeOr((inst.model * float4(localB, 0.0)).xyz,
                           float3(0.0, 0.0, 1.0));
    T = normalizeOr(T - N * dot(N, T), float3(1.0, 0.0, 0.0));
    B = normalizeOr(B - N * dot(N, B), cross(N, T));
    if (dot(cross(T, B), cross(T, B)) <= 1e-10) {
        float3x3 basis = buildOrthonormalBasis(N);
        T = basis[0];
        B = basis[1];
    }

    if (mat.normalTextureIndex >= 0 && uint(mat.normalTextureIndex) < textureCount) {
        float3 tangentNormal =
            sampleMaterialTexture(mat.normalTextureIndex, uv, materialTexture0,
                                  materialTexture1, materialTexture2,
                                  materialTexture3, materialTexture4,
                                  materialTexture5, materialTexture6,
                                  materialTexture7, materialTexture8,
                                  materialTexture9, materialTexture10,
                                  materialTexture11, materialTexture12,
                                  materialTexture13, materialTexture14,
                                  materialTexture15, materialTexture16,
                                  materialTexture17, materialTexture18,
                                  materialTexture19)
                .xyz;
        tangentNormal = normalizeOr(tangentNormal * 2.0 - 1.0,
                                    float3(0.0, 0.0, 1.0));
        N = normalizeOr(float3x3(T, B, N) * tangentNormal, N);
    }

    return N;
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
                      constant VertexData *vertices, constant uint *indices,
                      constant InstanceData *instanceData,
                      constant DirectionalLightData &dirLight,
                      constant SceneData &sceneData,
                      constant PointLight *pointLights,
                      constant SpotLight *spotLights,
                      constant AreaLight *areaLights,
                      texture2d<float> materialTexture0,
                      texture2d<float> materialTexture1,
                      texture2d<float> materialTexture2,
                      texture2d<float> materialTexture3,
                      texture2d<float> materialTexture4,
                      texture2d<float> materialTexture5,
                      texture2d<float> materialTexture6,
                      texture2d<float> materialTexture7,
                      texture2d<float> materialTexture8,
                      texture2d<float> materialTexture9,
                      texture2d<float> materialTexture10,
                      texture2d<float> materialTexture11,
                      texture2d<float> materialTexture12,
                      texture2d<float> materialTexture13,
                      texture2d<float> materialTexture14,
                      texture2d<float> materialTexture15,
                      texture2d<float> materialTexture16,
                      texture2d<float> materialTexture17,
                      texture2d<float> materialTexture18,
                      texture2d<float> materialTexture19) {
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

    float2 texUV = float2(vertices[i0].uv) * b0 + float2(vertices[i1].uv) * b1 +
                   float2(vertices[i2].uv) * b2;
    float3 localN = normalizeOr(float3(vertices[i0].normal) * b0 +
                                    float3(vertices[i1].normal) * b1 +
                                    float3(vertices[i2].normal) * b2,
                                float3(0.0, 1.0, 0.0));
    float3 localT = normalizeOr(float3(vertices[i0].tangent) * b0 +
                                    float3(vertices[i1].tangent) * b1 +
                                    float3(vertices[i2].tangent) * b2,
                                float3(1.0, 0.0, 0.0));
    float3 localB = normalizeOr(float3(vertices[i0].bitangent) * b0 +
                                    float3(vertices[i1].bitangent) * b1 +
                                    float3(vertices[i2].bitangent) * b2,
                                float3(0.0, 0.0, 1.0));
    float3 N = resolveShadingNormal(
        mat, texUV, localN, localT, localB, inst, sceneData.materialTextureCount,
        materialTexture0, materialTexture1, materialTexture2, materialTexture3,
        materialTexture4, materialTexture5, materialTexture6, materialTexture7,
        materialTexture8, materialTexture9, materialTexture10,
        materialTexture11, materialTexture12, materialTexture13,
        materialTexture14, materialTexture15, materialTexture16,
        materialTexture17, materialTexture18, materialTexture19);
    float3 P = primaryRay.origin + primaryRay.direction * hit.distance;
    float3 V = normalize(-primaryRay.direction);

    float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float3 emissive;
    resolveMaterialParameters(
        mat, texUV, sceneData.materialTextureCount, materialTexture0,
        materialTexture1, materialTexture2, materialTexture3, materialTexture4,
        materialTexture5, materialTexture6, materialTexture7, materialTexture8,
        materialTexture9, materialTexture10, materialTexture11,
        materialTexture12, materialTexture13, materialTexture14,
        materialTexture15, materialTexture16, materialTexture17,
        materialTexture18, materialTexture19, albedo, metallic, roughness, ao,
        emissive);
    float sssStrength = clamp(1.0 - mat.albedo.w, 0.0, 1.0) * (1.0 - metallic);
    float sssThickness = mix(0.25, 1.75, ao);

    float3 direct = evalDirectLightingPBR(
        isect, sceneAS, P, N, V, albedo, metallic, roughness,
        sssStrength, sssThickness, dirLight, sceneData, pointLights, spotLights,
        areaLights);

    float3 indirect = float3(0.0);

    if (sceneData.maxBounces > 0) {
        float3 F0 = mix(float3(0.04), albedo, metallic);
        float3 F_approx = F_Schlick(max(dot(N, V), 0.0), F0);

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
            float2 u = float2(rand(rng), rand(rng));
            float3 localH = sampleGGX(u, max(roughness, 0.001));
            float3 H_world = normalize(basis * localH);
            float3 bounceDir = reflect(-V, H_world);

            if (dot(bounceDir, N) <= 0.0)
                bounceDir = reflect(-V, N);

            bounceRay.direction = bounceDir;

            float NdotL2 = max(dot(N, bounceDir), 1e-4);
            float NdotV2 = max(dot(N, V), 1e-4);
            float3 Fs = F_Schlick(max(dot(V, H_world), 0.0), F0);
            float Gs = G_Smith(NdotV2, NdotL2, roughness);

            brdfWeight = (Fs * Gs / max(4.0 * NdotV2, 1e-4)) /
                         max(specProb, 1e-4);

        } else {
            float2 u = float2(rand(rng), rand(rng));
            float3 localBounce = cosineSampleHemisphere(u);
            bounceRay.direction = normalize(basis * localBounce);

            float3 kD = (1.0 - F_approx) * (1.0 - metallic);
            brdfWeight = (kD * albedo) / max(1.0 - specProb, 1e-4);
        }

        brdfWeight = clamp(brdfWeight, float3(0.0), float3(1.25));

        auto bounceHit = isect.intersect(bounceRay, sceneAS, 0xFF);

        if (bounceHit.type == intersection_type::none) {
            indirect = brdfWeight * skyColor(bounceRay.direction, 0.0) *
                       sceneData.indirectStrength;
        } else {
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
            float bb1 = bbary.x;
            float bb2 = bbary.y;

            float2 bUV = float2(vertices[bj0].uv) * bb0 +
                         float2(vertices[bj1].uv) * bb1 +
                         float2(vertices[bj2].uv) * bb2;
            float3 bLocalN = normalizeOr(float3(vertices[bj0].normal) * bb0 +
                                             float3(vertices[bj1].normal) * bb1 +
                                             float3(vertices[bj2].normal) * bb2,
                                         float3(0.0, 1.0, 0.0));
            float3 bLocalT = normalizeOr(float3(vertices[bj0].tangent) * bb0 +
                                             float3(vertices[bj1].tangent) * bb1 +
                                             float3(vertices[bj2].tangent) * bb2,
                                         float3(1.0, 0.0, 0.0));
            float3 bLocalB = normalizeOr(float3(vertices[bj0].bitangent) * bb0 +
                                             float3(vertices[bj1].bitangent) * bb1 +
                                             float3(vertices[bj2].bitangent) * bb2,
                                         float3(0.0, 0.0, 1.0));
            float3 bN = resolveShadingNormal(
                bmat, bUV, bLocalN, bLocalT, bLocalB, binst,
                sceneData.materialTextureCount, materialTexture0,
                materialTexture1, materialTexture2, materialTexture3,
                materialTexture4, materialTexture5, materialTexture6,
                materialTexture7, materialTexture8, materialTexture9,
                materialTexture10, materialTexture11, materialTexture12,
                materialTexture13, materialTexture14, materialTexture15,
                materialTexture16, materialTexture17, materialTexture18,
                materialTexture19);
            float3 bP =
                bounceRay.origin + bounceRay.direction * bounceHit.distance;
            float3 bV = -bounceRay.direction;

            float3 bAlbedo;
            float bMetallic;
            float bRoughness;
            float bAo;
            float3 bEmissive;
            resolveMaterialParameters(
                bmat, bUV, sceneData.materialTextureCount, materialTexture0,
                materialTexture1, materialTexture2, materialTexture3,
                materialTexture4, materialTexture5, materialTexture6,
                materialTexture7, materialTexture8, materialTexture9,
                materialTexture10, materialTexture11, materialTexture12,
                materialTexture13, materialTexture14, materialTexture15,
                materialTexture16, materialTexture17, materialTexture18,
                materialTexture19, bAlbedo, bMetallic, bRoughness, bAo,
                bEmissive);
            float bSssStrength =
                clamp(1.0 - bmat.albedo.w, 0.0, 1.0) * (1.0 - bMetallic);
            float bSssThickness = mix(0.25, 1.75, bAo);

            float3 bounceDirect = evalDirectLightingPBR(
                isect, sceneAS, bP, bN, bV, bAlbedo, bMetallic,
                bRoughness, bSssStrength, bSssThickness, dirLight, sceneData,
                pointLights, spotLights, areaLights);

            float3 bAmbient = bAlbedo * 0.04 * (1.0 - bMetallic) * bAo;
            indirect = brdfWeight * (bAmbient + bounceDirect + bEmissive) *
                       sceneData.indirectStrength;
        }

        indirect = clampLuminance(indirect, 3.0);
    }

    float3 ambient = albedo * 0.04 * (1.0 - metallic) * ao;
    return clampLuminance(ambient + direct + emissive + indirect, 10.0);
}

kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  texture2d<float, access::read> prevTex [[texture(1)]],
                  instance_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant MeshData *meshData [[buffer(3)]],
                  constant VertexData *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
                  constant SceneData &sceneData [[buffer(8)]],
                  constant PointLight *pointLights [[buffer(9)]],
                  constant SpotLight *spotLights [[buffer(10)]],
                  constant AreaLight *areaLights [[buffer(11)]],
                  texture2d<float> materialTexture0 [[texture(12)]],
                  texture2d<float> materialTexture1 [[texture(13)]],
                  texture2d<float> materialTexture2 [[texture(14)]],
                  texture2d<float> materialTexture3 [[texture(15)]],
                  texture2d<float> materialTexture4 [[texture(16)]],
                  texture2d<float> materialTexture5 [[texture(17)]],
                  texture2d<float> materialTexture6 [[texture(18)]],
                  texture2d<float> materialTexture7 [[texture(19)]],
                  texture2d<float> materialTexture8 [[texture(20)]],
                  texture2d<float> materialTexture9 [[texture(21)]],
                  texture2d<float> materialTexture10 [[texture(22)]],
                  texture2d<float> materialTexture11 [[texture(23)]],
                  texture2d<float> materialTexture12 [[texture(24)]],
                  texture2d<float> materialTexture13 [[texture(25)]],
                  texture2d<float> materialTexture14 [[texture(26)]],
                  texture2d<float> materialTexture15 [[texture(27)]],
                  texture2d<float> materialTexture16 [[texture(28)]],
                  texture2d<float> materialTexture17 [[texture(29)]],
                  texture2d<float> materialTexture18 [[texture(30)]],
                  texture2d<float> materialTexture19 [[texture(31)]],
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
                                           pointLights, spotLights, areaLights,
                                           materialTexture0, materialTexture1,
                                           materialTexture2, materialTexture3,
                                           materialTexture4, materialTexture5,
                                           materialTexture6, materialTexture7,
                                           materialTexture8, materialTexture9,
                                           materialTexture10, materialTexture11,
                                           materialTexture12, materialTexture13,
                                           materialTexture14, materialTexture15,
                                           materialTexture16, materialTexture17,
                                           materialTexture18, materialTexture19);
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
