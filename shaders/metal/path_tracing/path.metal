/*
 path.metal
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Path Tracing shader for rendering scenes
 Copyright (c) 2026 Max Van den Eynde
*/

#include <metal_stdlib>
#include <metal_raytracing>

using namespace metal;
using namespace raytracing;

struct Ray {
    float3 origin;
    float3 dir;
};

kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  acceleration_structure<> sceneAS [[buffer(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= outTex.get_width() || gid.y >= outTex.get_height())
        return;

    float2 uv =
        (float2(gid) + 0.5f) / float2(outTex.get_width(), outTex.get_height());
    float3 rayOrigin = float3(0.0f, 0.0f, 2.0f);
    float3 rd = normalize(float3(uv * 2.0 - 1.0, -1.0));

    intersector<triangle_data> isect;
    isect.assume_geometry_type(geometry_type::triangle);

    ray r;
    r.origin = rayOrigin;
    r.direction = rd;
    r.min_distance = 0.001f;
    r.max_distance = 1.0e30;

    intersection_result<triangle_data> hit = isect.intersect(r, sceneAS);

    float4 color = hit.type != intersection_type::none ? float4(1, 0, 0, 1)
                                                       : float4(0, 0, 0, 1);
    outTex.write(color, gid);
}
