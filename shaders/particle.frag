#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 FragColor;
uniform sampler2D particleTexture;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(particleTexture, fragTexCoord);
        if (texColor.a < 0.01) discard;
        FragColor = texColor * fragColor;
    } else {
        vec2 center = vec2(0.5, 0.5);
        float dist = distance(fragTexCoord, center);
        
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        
        FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
        
        if (FragColor.a < 0.01) discard;
    }
}