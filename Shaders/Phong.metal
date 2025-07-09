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

float4 sampleColorTextures(array<texture2d<float>, 3> color_textures,
                          sampler s,
                          float2 texCoords,
                          int textureCount,
                          float4 vertexColor) {
    if (textureCount == 0) {
        return vertexColor;
    }
    
    float4 baseColor = float4(0);
    for (uint i = 0; i < uint(textureCount); ++i) {
        baseColor += color_textures[i].sample(s, texCoords);
    }
    return baseColor / float(textureCount);
}

float3 sampleSpecularTextures(array<texture2d<float>, 2> specular_textures,
                             sampler s,
                             float2 texCoords,
                             int specularTextureCount) {
    if (specularTextureCount == 0) {
        return float3(1.0);
    }
    
    float3 specularMapColor = float3(0.0);
    for (uint i = 0; i < uint(specularTextureCount); ++i) {
        specularMapColor += specular_textures[i].sample(s, texCoords).rgb;
    }
    return specularMapColor / float(specularTextureCount);
}

float3 calculateDiffuse(float3 lightDir, float3 normal, float3 lightDiffuse, float3 materialDiffuse, float3 baseColor) {
    float diff = max(dot(normal, lightDir), 0.0);
    return lightDiffuse * (diff * materialDiffuse) * baseColor;
}

float3 calculateSpecular(float3 lightDir, float3 normal, float3 viewDir, float3 lightSpecular, float3 materialSpecular, float3 specularMapColor, float shininess) {
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    return lightSpecular * (spec * materialSpecular) * specularMapColor;
}

float3 calculatePointLight(Light light,
                          float3 fragPosition,
                          float3 normal,
                          float3 viewDir,
                          float3 baseColor,
                          float3 specularMapColor,
                          float3 materialDiffuse,
                          float3 materialSpecular,
                          float3 ambient,
                          float shininess) {
    float distance = length(light.position.xyz - fragPosition);
    float attenuation = 1.0 / (light.constantVal + light.linear * distance + light.quadratic * (distance * distance));
    
    float3 lightDir = normalize(light.position.xyz - fragPosition);
    
    float3 diffuse = calculateDiffuse(lightDir, normal, light.diffuse.rgb, materialDiffuse, baseColor) * attenuation;
    float3 specular = calculateSpecular(lightDir, normal, viewDir, light.specular.rgb, materialSpecular, specularMapColor, shininess) * attenuation;
    
    return (diffuse + specular + (ambient * attenuation)) * light.intensity;
}

float3 calculateDirectionalLight(Light light,
                                float3 normal,
                                float3 viewDir,
                                float3 baseColor,
                                float3 specularMapColor,
                                float3 materialDiffuse,
                                float3 materialSpecular,
                                float3 ambient,
                                float shininess) {
    float3 lightDir = normalize(-light.direction.xyz);
    
    float3 diffuse = calculateDiffuse(lightDir, normal, light.diffuse.rgb, materialDiffuse, baseColor);
    float3 specular = calculateSpecular(lightDir, normal, viewDir, light.specular.rgb, materialSpecular, specularMapColor, shininess);
    
    return (diffuse + specular + ambient) * light.intensity;
}

float3 calculateSpotlight(Light light,
                         float3 fragPosition,
                         float3 normal,
                         float3 viewDir,
                         float3 baseColor,
                         float3 specularMapColor,
                         float3 materialDiffuse,
                         float3 materialSpecular,
                         float3 ambient,
                         float shininess) {
    float3 lightDir = normalize(light.position.xyz - fragPosition);
    float3 spotDir = normalize(light.direction.xyz);
    
    float theta = dot(lightDir, -spotDir);
    
    if (theta > light.outerCutoff) {
        float epsilon = light.innerCutoff - light.outerCutoff;
        float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
        
        float distance = length(light.position.xyz - fragPosition);
        float attenuation = 1.0 / (light.constantVal + light.linear * distance + light.quadratic * (distance * distance));
        
        float3 diffuse = calculateDiffuse(lightDir, normal, light.diffuse.rgb, materialDiffuse, baseColor) * attenuation * intensity;
        float3 specular = calculateSpecular(lightDir, normal, viewDir, light.specular.rgb, materialSpecular, specularMapColor, shininess) * attenuation * intensity;
        
        return (diffuse + specular + (ambient * attenuation * intensity)) * light.intensity;
    }
    
    return float3(0.0);
}

fragment float4 phong_fragment(VertexOut in [[stage_in]],
                               constant PhongUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               array<texture2d<float>, 2> specular_textures [[ texture(3) ]],
                               constant Light *lights [[ buffer(2) ]],
                               sampler s [[ sampler(0) ]]) {
    
    float4 baseColor = sampleColorTextures(color_textures, s, in.texCoords, uniforms.textureCount, in.color);
    if (baseColor.a < 0.1) {
        discard_fragment();
    }
    float3 specularMapColor = sampleSpecularTextures(specular_textures, s, in.texCoords, uniforms.specularTextureCount);
    
    float3 ambient = uniforms.ambientColor.rgb * uniforms.material.ambient * baseColor.rgb;
    
    float3 result = float3(0);
    float3 norm = normalize(in.normals);
    float3 viewDir = normalize(uniforms.cameraPos - in.fragPosition);
    
    for (int i = 0; i < uniforms.lightCount; ++i) {
        if (lights[i].type == POINT_LIGHT) {
            result += calculatePointLight(lights[i], in.fragPosition, norm, viewDir, baseColor.rgb,
                                        specularMapColor, uniforms.material.diffuse, uniforms.material.specular,
                                        ambient, uniforms.material.shininess);
        }
        else if (lights[i].type == DIRECTIONAL_LIGHT) {
            result += calculateDirectionalLight(lights[i], norm, viewDir, baseColor.rgb,
                                              specularMapColor, uniforms.material.diffuse, uniforms.material.specular,
                                              ambient, uniforms.material.shininess);
        }
        else if (lights[i].type == SPOTLIGHT) {
            result += calculateSpotlight(lights[i], in.fragPosition, norm, viewDir, baseColor.rgb,
                                       specularMapColor, uniforms.material.diffuse, uniforms.material.specular,
                                       ambient, uniforms.material.shininess);
        }
    }
    
    return float4(result, baseColor.a);
}
