#version 410 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inVelocity;
layout(location = 2) in float inDensity;
layout(location = 3) in float inMass;

out vec3 outPosition;
out vec3 outVelocity;
out float outDensity;
out float outMass;

uniform float dt;
uniform float gravity;
uniform vec3 bounds;
uniform float particleSize;

struct Movement {
    vec3 position;
    vec3 velocity;
};

struct Particle {
    vec3 position;
    vec3 velocity;
    float density;
    float mass;
};

Particle resolveCollisions(Particle particle) {
    // Check against 0 and bounds directly
    if (particle.position.x < particleSize) {
        particle.position.x = particleSize;
        particle.velocity.x *= -1.0;
    }
    if (particle.position.x > bounds.x - particleSize) {
        particle.position.x = bounds.x - particleSize;
        particle.velocity.x *= -1.0;
    }

    if (particle.position.y < particleSize) {
        particle.position.y = particleSize;
        particle.velocity.y *= -1.0;
    }
    if (particle.position.y > bounds.y - particleSize) {
        particle.position.y = bounds.y - particleSize;
        particle.velocity.y *= -1.0;
    }

    if (particle.position.z < particleSize) {
        particle.position.z = particleSize;
        particle.velocity.z *= -1.0;
    }
    if (particle.position.z > bounds.z - particleSize) {
        particle.position.z = bounds.z - particleSize;
        particle.velocity.z *= -1.0;
    }

    return particle;
}

void main() {
    Particle p;
    p.position = inPosition;
    p.velocity = inVelocity;
    p.density = inDensity;
    p.mass = inMass;

    outPosition = inPosition;
    outVelocity = inVelocity;
    outDensity = inDensity;
    outMass = inMass;

    // Apply gravity
    p.velocity.y -= gravity * dt;

    // Add velocity to position
    p.position += p.velocity * dt;

    // Check collisions
    p = resolveCollisions(p);

    outPosition = p.position;
    outVelocity = p.velocity;
    outDensity = p.density;
    outMass = p.mass;
}
