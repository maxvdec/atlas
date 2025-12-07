#version 450
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoord;

layout(set = 2, binding = 0) uniform sampler2D image;

layout(set = 1, binding = 0) uniform Params {
    bool horizontal;
    float weight[5];
    float radius;
};

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0) * radius; 
    vec3 result = texture(image, TexCoord).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}