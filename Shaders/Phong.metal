//
//  Phong.metal
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

vertex VertexOut phong_vertex(Vertex in [[stage_in]], constant PhongUniforms &uniforms [[ buffer(1)]]) {
    VertexOut out;
    float4 position = in.position;
    float4x4 mvp = uniforms.projection * uniforms.view * uniforms.model;
    position = mvp * position;
    out.position = position;
    out.color = in.color;
    out.texCoords = in.texCoords;
    
    float3x3 normalMatrix = transpose3x3(inverse3x3(toFloat3x3(uniforms.model)));
    float3 transformedNormal = normalize(normalMatrix * in.normals.xyz);
    out.normals = transformedNormal;
    
    out.fragPosition = (uniforms.model * in.position).xyz;
    
    return out;
}

fragment float4 phong_fragment(VertexOut in [[stage_in]],
                               constant PhongUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               array<texture2d<float>, 2> specular_textures [[ texture(3) ]],
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

    float3 ambient = uniforms.ambientColor.rgb * uniforms.material.ambient * baseColor.rgb;
    float3 result = float3(0);
    
    float3 norm = normalize(in.normals);
    
    float3 specularMapColor = float3(1.0);

    if (uniforms.specularTextureCount > 0) {
        specularMapColor = float3(0.0);
        for (uint i = 0; i < uint(uniforms.specularTextureCount); ++i) {
            specularMapColor += specular_textures[i].sample(s, in.texCoords).rgb;
        }
        specularMapColor /= float(uniforms.specularTextureCount);
    }

    
    for (int i = 0; i < uniforms.lightCount; ++i) {
        if (lights[i].type == POINT_LIGHT) {
            float distance = length(lights[i].position.xyz - in.fragPosition);
            float attenuation = 1.0 / (lights[i].constantVal + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
            
            float3 lightDir = normalize(lights[i].position.xyz - in.fragPosition);
            float diff = max(dot(norm, lightDir), 0.0);
            float3 diffuse = lights[i].diffuse.rgb * (diff * uniforms.material.diffuse) * baseColor.rgb * attenuation;
            
            float3 viewDir = normalize(uniforms.cameraPos - in.fragPosition);
            float3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), uniforms.material.shininess);
            float3 specular = lights[i].specular.rgb * (spec * uniforms.material.specular) * specularMapColor * attenuation;
            
            result += (diffuse + specular + (ambient * attenuation)) * lights[i].intensity;
        } else if (lights[i].type == DIRECTIONAL_LIGHT) {
            float3 lightDir = normalize(-lights[i].direction.xyz);
            float diff = max(dot(norm, lightDir), 0.0);
            float3 diffuse = lights[i].diffuse.rgb * (diff * uniforms.material.diffuse) * baseColor.rgb;
            
            float3 viewDir = normalize(uniforms.cameraPos - in.fragPosition);
            float3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), uniforms.material.shininess);
            float3 specular = lights[i].specular.rgb * (spec * uniforms.material.specular) * specularMapColor;
            
            result += (diffuse + specular + ambient) * lights[i].intensity;
        } else if (lights[i].type == SPOTLIGHT) {
            float3 lightDir = normalize(lights[i].position.xyz - in.fragPosition);
            float3 spotDir = normalize(lights[i].direction.xyz);
            
            float theta = dot(lightDir, -spotDir);
            
            if (theta > lights[i].outerCutoff) {
                float epsilon = lights[i].innerCutoff - lights[i].outerCutoff;
                float intensity = clamp((theta - lights[i].outerCutoff) / epsilon, 0.0, 1.0);
                    
                float distance = length(lights[i].position.xyz - in.fragPosition);
                float attenuation = 1.0 / (lights[i].constantVal + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
                    
                float diff = max(dot(norm, lightDir), 0.0);
                float3 diffuse = lights[i].diffuse.rgb * (diff * uniforms.material.diffuse) * baseColor.rgb * attenuation * intensity;
                    
                float3 viewDir = normalize(uniforms.cameraPos - in.fragPosition);
                float3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), uniforms.material.shininess);
                float3 specular = lights[i].specular.rgb * (spec * uniforms.material.specular) * specularMapColor * attenuation * intensity;
                    
                result += (diffuse + specular + (ambient * attenuation * intensity)) * lights[i].intensity;
            }
        }
    }
    
    return float4(result, baseColor.a);
}
