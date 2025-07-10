//
//  Depth.metal
//  Atlas
//
//  Created by Max Van den Eynde on 10/7/25.
//

#include <metal_stdlib>
using namespace metal;

vertex DepthVertexOut depth_vertex(DepthVertexIn in [[stage_in]], constant float4x4 &lightVP [[buffer(1)]], constant float4x4 &model [[buffer(2)]] ) {
    DepthVertexOut out;
    float4 worldPos = model * float4(in.position, 1.0);
    out.position = lightVP * worldPos;
    return out;
}

