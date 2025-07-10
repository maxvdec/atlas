//
//  Shadow.metal - Fixed Version
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

vertex VertexOut shadow_vertex(Vertex in [[stage_in]], constant ShadowUniforms &uniforms [[ buffer(1)]]) {
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
    if (uniforms.casters > 0) {
        out.fragPositionLightSpace = uniforms.lightSpace * float4(out.fragPosition, 1.0);
    }
    
    return out;
}

float calculate_shadow(float4 fragPosLightSpace, ShadowUniforms uniforms, depth2d<float> depthMap, sampler s, float3 normals, Light light) {
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }
    
    float closestDepth = depthMap.sample(s, projCoords.xy);
    float currentDepth = projCoords.z;
    
    float3 lightDir = normalize(light.direction.xyz);
    float3 normal = normalize(normals);
    float cosTheta = clamp(dot(lightDir, normal), 0.0, 1.0);
    float bias = 0.001 * tan(acos(cosTheta));
    bias = clamp(bias, 0.0001, 0.005);
    
    float shadow = 0.0;
    float2 texelSize = 1.0 / 1024.0;
    
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = depthMap.sample(s, projCoords.xy + float2(x, y) * texelSize);
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

float calculate_shadow_simple(float4 fragPosLightSpace, ShadowUniforms uniforms, depth2d<float> depthMap, sampler s, float3 normals, Light light) {
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }
    
    float closestDepth = depthMap.sample(s, projCoords.xy);
    float currentDepth = projCoords.z;
    
    float3 lightDir = normalize(light.direction.xyz);
    float3 normal = normalize(normals);
    float cosTheta = clamp(dot(lightDir, normal), 0.0, 1.0);
    float bias = 0.0005 + 0.0005 * tan(acos(cosTheta));
    bias = clamp(bias, 0.0001, 0.002);
    
    float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;
    return shadow;
}

fragment float4 shadow_fragment(VertexOut in [[stage_in]],
                               constant ShadowUniforms &uniforms [[ buffer(1)]],
                               array<texture2d<float>, 3> color_textures [[ texture(0) ]],
                               array<texture2d<float>, 2> specular_textures [[ texture(3) ]],
                               depth2d<float> depth_texture [[ texture(5)]],
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
    
    Light mainCaster;
    
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
        
        if (lights[i].casts) {
            mainCaster = lights[i];
        }
    }
    
    float shadow = 1.0;
    
    if (uniforms.casters > 0) {
        shadow = calculate_shadow(in.fragPositionLightSpace, uniforms, depth_texture, s, in.normals, mainCaster);
    }
    
    result *= shadow;
    
    return float4(result, 1.0);
}

fragment float4 debug_shadow_fragment(VertexOut in [[stage_in]],
                                     constant ShadowUniforms &uniforms [[ buffer(1)]],
                                     depth2d<float> depth_texture [[ texture(5)]],
                                     sampler s [[ sampler(0) ]]) {
    
    if (uniforms.casters > 0) {
        float3 projCoords = in.fragPositionLightSpace.xyz / in.fragPositionLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        
        if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z < 0.0 || projCoords.z > 1.0) {
            return float4(1.0, 0.0, 0.0, 1.0);
        }
        
        float closestDepth = depth_texture.sample(s, projCoords.xy);
        float currentDepth = projCoords.z;
        
        if (currentDepth > closestDepth + 0.001) {
            return float4(0.0, 0.0, 1.0, 1.0);
        } else {
            return float4(0.0, 1.0, 0.0, 1.0);
        }
    }
    
    return float4(0.5, 0.5, 0.5, 1.0);
}
