#version 330 core
out vec4 FragColor;
  
in vec2 textCoord;

uniform sampler2D uTexture1;

void main()
{             
    float depthValue = texture(uTexture1, textCoord).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}  
