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

float3 evalDirectionalLight(DirectionalLightData light, float3 P, float3 N,
                            float3 albedo) {
    float3 L = normalize(-light.direction);
    return lambert(albedo, N, L, light.color, max(light.intensity, 0.0));
}

bool isOccludedDirectionalLight(DirectionalLightData light, float3 P, float3 N,
                                intersector<triangle_data, instancing> isect,
                                instance_acceleration_structure sceneAS) {
    float3 L = normalize(-light.direction);

    return isOccluded(isect, sceneAS, P, N, L, 1e30);
}

float3 evalPointLight(PointLight light, float3 P, float3 N, float3 albedo) {
    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 0.001);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;

    float NdotL = max(dot(N, L), 0.0);

    float attenuation = 1.0 / dist2;

    float rangeFade = 1.0 - smoothstep(light.range * 0.8, light.range, dist);

    return albedo * light.color * light.intensity * NdotL * attenuation *
           rangeFade;
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

float3 evalSpotLight(SpotLight light, float3 P, float3 N, float3 albedo) {
    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;

    float NdotL = max(dot(N, L), 0.0);

    float3 lightForward = normalize(light.direction);
    float spotCos = dot(-L, lightForward);

    float spotFactor = smoothstep(light.outerCos, light.innerCos, spotCos);

    float attenuation = 1.0 / dist2;
    float rangeFade = 1.0 - smoothstep(light.range * 0.8, light.range, dist);

    return albedo * light.color * light.intensity * NdotL * attenuation *
           rangeFade * spotFactor;
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

float3 evalAreaLight(AreaLight light, float3 P, float3 N, float3 albedo) {
    float3 lightNormal = normalize(cross(light.right, light.up));

    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);

    float3 L = toLight / dist;

    float NdotL = max(dot(N, L), 0.0);

    float cosLight = dot(lightNormal, -L);
    if (light.twoSided > 0.5)
        cosLight = abs(cosLight);
    else
        cosLight = max(cosLight, 0.0);

    float area = 4.0 * light.halfWidth * light.halfHeight;

    float attenuation = 1.0 / dist2;

    return albedo * light.color * (light.intensity) * NdotL * cosLight * area *
           attenuation;
}

bool isOccludedAreaLight(AreaLight light, float3 P, float3 N,
                         intersector<triangle_data, instancing> isect,
                         instance_acceleration_structure sceneAS) {
    float3 lightNormal = normalize(cross(light.right, light.up));

    float3 toLight = light.position - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);

    float3 L = toLight / dist;

    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

float3 evalDirectLighting(intersector<triangle_data, instancing> isect,
                          instance_acceleration_structure sceneAS, float3 P,
                          float3 N, float3 albedo,
                          constant DirectionalLightData &dirLight,
                          constant SceneData &sceneData,
                          constant PointLight *pointLights,
                          constant SpotLight *spotLights,
                          constant AreaLight *areaLights) {
    float3 lighting = float3(0.0);

    if (sceneData.numDirectionalLights > 0) {
        float3 contribution = evalDirectionalLight(dirLight, P, N, albedo);
        if (isOccludedDirectionalLight(dirLight, P, N, isect, sceneAS)) {
            contribution = float3(0.0);
        }
        lighting += contribution;
    }

    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 contribution = evalPointLight(pointLights[i], P, N, albedo);
        if (isOccludedPointLight(pointLights[i], P, N, isect, sceneAS)) {
            contribution = float3(0.0);
        }
        lighting += contribution;
    }

    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 contribution = evalSpotLight(spotLights[i], P, N, albedo);
        if (isOccludedSpotLight(spotLights[i], P, N, isect, sceneAS)) {
            contribution = float3(0.0);
        }
        lighting += contribution;
    }

    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float3 contribution = evalAreaLight(areaLights[i], P, N, albedo);
        if (isOccludedAreaLight(areaLights[i], P, N, isect, sceneAS)) {
            contribution = float3(0.0);
        }
        lighting += contribution;
    }

    return lighting;
}

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

    float3 n0 = float3(vertices[i0]);
    float3 n1 = float3(vertices[i1]);
    float3 n2 = float3(vertices[i2]);

    float2 bary = hit.triangle_barycentric_coord;
    float b0 = 1.0 - bary.x - bary.y;
    float b1 = bary.x;
    float b2 = bary.y;

    float3 localN = normalize(n0 * b0 + n1 * b1 + n2 * b2);

    float3x3 normalMatrix =
        float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
    float3 N = normalize(normalMatrix * localN);

    float3 P = primaryRay.origin + primaryRay.direction * hit.distance;

    float3 direct =
        evalDirectLighting(isect, sceneAS, P, N, mat.albedo.xyz, dirLight,
                           sceneData, pointLights, spotLights, areaLights);

    float3 indirect = float3(0.0);

    if (sceneData.maxBounces > 0) {
        float2 u = float2(rand(rng), rand(rng));
        float3 localBounce = cosineSampleHemisphere(u);
        float3x3 basis = buildOrthonormalBasis(N);
        float3 bounceDir = normalize(basis * localBounce);

        ray bounceRay;
        bounceRay.origin = P + N * 0.001;
        bounceRay.direction = bounceDir;
        bounceRay.min_distance = 0.0;
        bounceRay.max_distance = 1.0e30;

        auto bounceHit = isect.intersect(bounceRay, sceneAS, 0xFF);

        if (bounceHit.type == intersection_type::none) {
            indirect = mat.albedo.xyz * skyColor(bounceDir, 0.0) *
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

            float3 bn0 = float3(vertices[bj0]);
            float3 bn1 = float3(vertices[bj1]);
            float3 bn2 = float3(vertices[bj2]);

            float2 bbary = bounceHit.triangle_barycentric_coord;
            float bb0 = 1.0 - bbary.x - bbary.y;
            float bb1 = bbary.x;
            float bb2 = bbary.y;

            float3 blocN = normalize(bn0 * bb0 + bn1 * bb1 + bn2 * bb2);

            float3x3 bNormalMatrix =
                float3x3(binst.normalCol0.xyz, binst.normalCol1.xyz,
                         binst.normalCol2.xyz);
            float3 bN = normalize(bNormalMatrix * blocN);

            float3 bP =
                bounceRay.origin + bounceRay.direction * bounceHit.distance;

            float3 bounceDirect = evalDirectLighting(
                isect, sceneAS, bP, bN, bmat.albedo.xyz, dirLight, sceneData,
                pointLights, spotLights, areaLights);

            float cosineTerm = max(dot(N, bounceDir), 0.0);
            indirect = mat.albedo.xyz * bounceDirect * cosineTerm *
                       sceneData.indirectStrength;
        }
    }

    float3 ambient = mat.albedo.xyz * 0.04;
    return ambient + direct + indirect;
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

            color += sampleRadiance(gid, s, w, isect, sceneAS, primaryRay,
                                    materials, meshData, vertices, indices,
                                    instanceData, dirLight, sceneData,
                                    pointLights, spotLights, areaLights);
        }

        color /= float(spp);

        int frameIndex = int(sceneData.frameIndex);

        float4 prevColor = prevTex.read(gid);
        if (frameIndex == 0) {
            prevColor = float4(0, 0, 0, 1);
        }
        float3 accum = (prevColor.xyz * frameIndex + color) / (frameIndex + 1);

        outTex.write(float4(accum, 1.0), gid);
    }
}
