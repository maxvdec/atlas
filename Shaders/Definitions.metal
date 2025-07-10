//
//  Definitions.metal
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

#include <metal_stdlib>
#include <metal_matrix>
using namespace metal;

struct Vertex {
    float4 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float2 texCoords [[attribute(2)]];
    float4 normals [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 fragPosition;
    float4 color;
    float2 texCoords;
    float3 normals;
};

struct BasicUniforms {
    int textureCount;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct Material {
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float shininess;
};

struct PhongUniforms {
    int textureCount;
    int specularTextureCount;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 ambientColor;
    int lightCount;
    float3 cameraPos;
    Material material;
};

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOTLIGHT 2

struct Light {
    uint type;
    float4 color;
    float4 position; // Used in: PointLight, SpotLight
    float4 direction; // Used in: DirectionalLight, Spotlight
    float intensity;
    
    float4 specular;
    float4 diffuse;
    
    float constantVal; // Used in: PointLight
    float linear; // Used in: PointLight
    float quadratic; // Used in: PointLight
    
    float innerCutoff; // Used in: SpotLight
    float outerCutoff;
};

#define COLOR_TEXTURE 0
#define SPECULAR_MAP 1
#define DEPTH_TEXTURE 2

struct DepthVertexIn {
    float3 position [[attribute(0)]];
};

struct DepthVertexOut {
    float4 position [[position]];
};
