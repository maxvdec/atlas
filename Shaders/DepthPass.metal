//
//  DepthPass.metal
//  Atlas
//
//  Created by Max Van den Eynde on 9/7/25.
//

#include <metal_stdlib>
using namespace metal;

vertex DepthPassVertexOut depth_vertex(DepthPassVertexIn in [[stage_in]],
                           constant float4x4 &modelMatrix [[buffer(1)]],
                           constant float4x4 &lightVP [[buffer(2)]]) {
    DepthPassVertexOut out;
    float4 worldPos = modelMatrix * in.position;
    out.position = lightVP * worldPos;
    return out;
}

fragment float depth_fragment(DepthPassVertexOut in [[stage_in]]) {
    return in.position.z / in.position.w;
}
