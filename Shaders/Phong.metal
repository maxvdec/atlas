//
//  Phong.metal
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

#include <metal_stdlib>
using namespace metal;

vertex VertexOut phong_vertex(Vertex in [[stage_in]], constant PhongUniforms &uniforms [[ buffer(1)]]) {
    VertexOut out;
    float4 position = float4(in.position, 1.0);
    float4x4 mvp = uniforms.projection * uniforms.view * uniforms.model;
    position = mvp * position;
    out.position = position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    float3x3 normalMatrix = transpose(inverse3x3(toFloat3x3(uniforms.model)));
    float3 transformedNormal = normalize(normalMatrix * in.normals);
    out.normals = transformedNormal;
    out.fragPosition = (uniforms.model * float4(in.position, 1.0)).xyz;
    return out;
}

fragment float4 phong_fragment(VertexOut in [[stage_in]],
                               constant PhongUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               constant Light *lights [[ buffer(2) ]],
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
    
    finalColor *= uniforms.ambientColor;
    return finalColor;
}


