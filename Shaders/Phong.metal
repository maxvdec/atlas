//
//  Phong.metal
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

vertex VertexOut phong_vertex(Vertex in [[stage_in]], constant PhongUniforms &uniforms [[ buffer(1)]]) {
    VertexOut out;
    float4 position = float4(in.position, 1.0);
    float4x4 mvp = uniforms.projection * uniforms.view * uniforms.model;
    position = mvp * position;
    out.position = position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    
    out.normals = in.normals;
    
    out.fragPosition = (uniforms.model * float4(in.position, 1.0)).xyz;
    
    return out;
}

fragment float4 phong_fragment(VertexOut in [[stage_in]],
                               constant PhongUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               constant Light *lights [[ buffer(2) ]],
                               sampler s [[ sampler(0) ]]) {
    
    float4 baseColor = float4(0);
    if (uniforms.textureCount != 0) {
        for (uint i = 0; i < uint(uniforms.textureCount); ++i) {
            baseColor += color_textures[i].sample(s, in.texCoords);
        }
        baseColor /= float(uniforms.textureCount);
    } else {
        baseColor = in.color;
    }

    float3 ambient = uniforms.ambientColor.rgb * uniforms.material.ambient;
    float3 result = 0;
    
    float3 norm = in.normals;
    return float4((in.normals * 0.5 + 0.5), 1.0);
    
    for (int i = 0; i < uniforms.lightCount; ++i) {
        float3 lightDir = normalize(lights[i].position.xyz - in.fragPosition);
        float diff = max(dot(norm, lightDir), 0.0);
        float3 diffuse = lights[i].diffuse.rgb * (diff * uniforms.material.diffuse) * lights[i].intensity;
        
        float3 viewDir = normalize(uniforms.cameraPos - in.fragPosition);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), uniforms.material.shininess);
        float3 specular = lights[i].specular.rgb * (spec * uniforms.material.specular) * lights[i].intensity;
        
        result += ambient + diffuse + specular;
    }
    
    return float4(result, 1.0);
}
