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

kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  instance_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant MeshData *meshData [[buffer(3)]],
                  constant packed_float3 *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
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

        float3 localN = n0 * b0 + n1 * b1 + n2 * b2;
        float3x3 normalMatrix =
            float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
        float3 Ns = normalize(normalMatrix * localN);

        float3 L = normalize(-dirLight.direction);
        float ndl = max(dot(Ns, L), 0.0);

        float3 diffuse =
            mat.albedo.xyz * dirLight.color * max(dirLight.intensity, 0.0) * ndl;
        float3 ambient = mat.albedo.xyz * 0.04;
        float3 color = diffuse + ambient;

        outTex.write(float4(color, 1), gid);
    }
}
