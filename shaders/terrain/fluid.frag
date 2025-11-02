#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 WorldPos;          
in vec3 WorldNormal;      

uniform vec4 waterColor;         
uniform sampler2D sceneTexture;  
uniform sampler2D sceneDepth;    
uniform sampler2D normalMap;     
uniform vec3 cameraPos;           
uniform float time;
uniform float refractionStrength; 
uniform float reflectionStrength; 
uniform float depthFade;         
uniform mat4 projection;
uniform mat4 view;
uniform mat4 invProjection;      
uniform mat4 invView;

void main()
{
    vec4 sceneColor = texture(sceneTexture, TexCoord);
    FragColor = sceneColor;
}