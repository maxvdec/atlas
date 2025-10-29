#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aVelocity;
layout(location = 2) in float aDensity;
layout(location = 3) in float aMass;

out vec3 outPosition;
out vec3 outVelocity;
out float outDensity;

void main() {
    outPosition = aPos;
    outVelocity = aVelocity;
    outDensity = aDensity;
}
