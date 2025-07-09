//
//  Fullscreen.metal
//  Atlas
//
//  Created by Max Van den Eynde on 9/7/25.
//

#include <metal_stdlib>
using namespace metal;

vertex VertexOut fullscreen_vertex(Vertex in [[stage_in]]) {
    VertexOut out;
    out.position = in.position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    out.normals = in.normals.xyz;
    out.fragPosition = float3(1);
    return out;
}

fragment float4 fullscreen_fragment(VertexOut in [[stage_in]],
                               texture2d<float> texture [[ texture(0) ]],
                               sampler s [[ sampler(0) ]],
                               constant uint &textureType [[ buffer(0) ]]) {
    if (textureType == COLOR_TEXTURE) {
        return texture.sample(s, in.texCoords);
    } else if (textureType == DEPTH_TEXTURE) {
        float depth = texture.sample(s, in.texCoords).r;
        return float4(depth, depth, depth, 1.0);
    } else {
        return texture.sample(s, in.texCoords);
    }
}

