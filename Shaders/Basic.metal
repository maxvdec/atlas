//
//  Basic.metal
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

#include <metal_stdlib>
using namespace metal;

vertex VertexOut basic_vertex(Vertex in [[stage_in]], constant BasicUniforms &uniforms [[ buffer(1)]]) {
    VertexOut out;
    float4 position = in.position;
    float4x4 mvp = uniforms.projection * uniforms.view * uniforms.model;
    position = mvp * position;
    out.position = position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    float3x3 normalMatrix = transpose(inverse3x3(toFloat3x3(uniforms.model)));
    float3 transformedNormal = normalize(normalMatrix * in.normals.xyz);
    out.normals = transformedNormal;
    out.fragPosition = (uniforms.model * in.position).xyz;
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
    if (finalColor.a < 0.1) {
        discard_fragment();
    }
    return finalColor;
}


