#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;   
uniform sampler2D gNormal;
uniform sampler2D gMaterial; // metallic, roughness, reflectivity

uniform mat4 projection;
uniform mat4 view;

const float maxDistance = 8;
const float resolution = 0.3;
const int steps = 5;
const float thickness = 0.5;

void main() {
    vec4 uv = vec4(0.0);

    vec4 positionFrom = texture(gPosition, TexCoord);
    vec4 mask = texture(gMaterial, TexCoord);

    if (positionFrom.w <= 0 || mask.b <= 0.0) { FragColor = uv; return; }

    vec3 unitPositionFrom = normalize(positionFrom.xyz);
    vec3 normal = normalize(view * vec4(texture(gNormal, TexCoord).xyz, 0.0)).xyz;
    vec3 pivot = normalize(reflect(unitPositionFrom, normal));

    vec4 positionTo = positionFrom;

    vec4 startView = vec4(positionFrom.xyz + (pivot * 0.0), 1.0);
    vec4 endView = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);
    
    vec4 startFrag = projection * startView;
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5 + 0.5;

    vec4 endFrag = projection * endView;
    endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5 + 0.5;

    vec2 frag = startFrag.xy;

    float deltaX = endFrag.x - startFrag.x;
    float deltaY = endFrag.y - startFrag.y;
    float useX = abs(deltaX) > abs(deltaY) ? 1.0 : 0.0;
    float delta = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
    vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.001);

    float search0 = 0;
    float search1 = 0;

    int hit0 = 0;
    int hit1 = 0;

    float viewDistance = startView.y;
    float depth = thickness;

    float i = 0;
    for (i = 0; i < int(delta); ++i) {
        frag += increment;
        uv.xy = frag;
        positionTo = view * texture(gPosition, uv.xy);

        search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
        search1 = clamp(search1, 0.0, 1.0);

        viewDistance = (startView.y * endView.y) / mix(startView.y, endView.y, search1);
        depth = viewDistance - positionTo.y;

        if (depth > 0 && depth < thickness) {
            hit0 = 1;
            break;
        } else {
            search0 = search1;
        }
    }

}
