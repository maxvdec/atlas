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
    float _pad0[3];
};

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

    return albedo * light.color * (light.intensity * 2) * NdotL * cosLight *
           area * attenuation;
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

        float3x3 normalMatrix = float3x3(
            inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
        float3 N = normalize(normalMatrix * localN);

        float t = hit.distance;
        float3 P = r.origin + r.direction * t;

        float3 lighting = float3(0.0);

        if (sceneData.numDirectionalLights > 0) {
            float3 contribution =
                evalDirectionalLight(dirLight, P, N, mat.albedo.xyz);
            bool blocking =
                isOccludedDirectionalLight(dirLight, P, N, isect, sceneAS);
            if (blocking) {
                contribution = float3(0.0);
            }
            lighting += contribution;
        }

        for (uint i = 0; i < sceneData.numPointLights; ++i) {
            float3 contribution =
                evalPointLight(pointLights[i], P, N, mat.albedo.xyz);
            bool blocking =
                isOccludedPointLight(pointLights[i], P, N, isect, sceneAS);
            if (blocking) {
                contribution = float3(0.0);
            }
            lighting += contribution;
        }

        for (uint i = 0; i < sceneData.numSpotLights; ++i) {
            float3 contribution =
                evalSpotLight(spotLights[i], P, N, mat.albedo.xyz);
            bool blocking =
                isOccludedSpotLight(spotLights[i], P, N, isect, sceneAS);
            if (blocking) {
                contribution = float3(0.0);
            }
            lighting += contribution;
        }

        for (uint i = 0; i < sceneData.numAreaLights; ++i) {
            float3 contribution =
                evalAreaLight(areaLights[i], P, N, mat.albedo.xyz);
            bool blocking =
                isOccludedAreaLight(areaLights[i], P, N, isect, sceneAS);
            if (blocking) {
                contribution = float3(0.0);
            }
            lighting += contribution;
        }

        float3 ambient = mat.albedo.xyz * 0.04;
        float3 color = ambient + lighting;

        int frameIndex = int(sceneData.frameIndex);

        float4 prevColor = prevTex.read(gid);
        if (frameIndex == 0) {
            prevColor = float4(0, 0, 0, 1);
        }
        float3 accum = (prevColor.xyz * frameIndex + color) / (frameIndex + 1);

        outTex.write(float4(accum, 1.0), gid);
    }
}
