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
    float2 texCoords [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float2 texCoords;
};

struct BasicUniforms {
    int textureCount;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut basic_vertex(Vertex in [[stage_in]], constant BasicUniforms &uniforms [[ buffer(1)]]) {
    VertexOut out;
    float4 position = float4(in.position, 1.0);
    float4x4 mvp = uniforms.projection * uniforms.view * uniforms.model;
    position = mvp * position;
    out.position = position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    return out;
}

fragment float4 basic_fragment(VertexOut in [[stage_in]],
                               constant BasicUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               sampler s [[ sampler(0) ]]) {
    float4 finalColor = float4(0);
    if (uniforms.textureCount != 0) {
        for (uint i = 0; i < uint(uniforms.textureCount); ++i) {
            finalColor += color_textures[i].sample(s, in.texCoords);
        }
        finalColor /= float(uniforms.textureCount); // average
    } else {
        finalColor = in.color;
    }
    return finalColor;
}


