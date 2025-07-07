//
//  Basic.metal
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

#include <metal_stdlib>
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut basic_vertex(Vertex in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 basic_fragment(VertexOut in [[stage_in]]) {
    return in.color;
}

