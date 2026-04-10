import { GameObject } from "atlas";
import { Position3d } from "atlas/units";

export const ParticleEmissionType = Object.freeze({
    Fountain: 0,
    Ambient: 1,
});

export class ParticleEmitter extends GameObject {
    constructor(maxParticles = 100) {
        super();
        this.settings = {
            minLifetime: 1,
            maxLifetime: 3,
            minSize: 0.02,
            maxSize: 0.01,
            fadeSpeed: 0.5,
            gravity: -9.81,
            spread: 1,
            speedVariation: 1,
        };
        this.position = Position3d.zero();
        this.emissionType = ParticleEmissionType.Fountain;
        this.direction = Position3d.up();
        this.spawnRadius = 0.1;
        this.spawnRate = 10;
        globalThis.__atlasCreateParticleEmitter(this, maxParticles);
    }

    attachTexture(texture) {
        return globalThis.__atlasAttachParticleEmitterTexture(this, texture);
    }

    setColor(color) {
        return globalThis.__atlasSetParticleEmitterColor(this, color);
    }

    enableTexture() {
        return globalThis.__atlasSetParticleEmitterUseTexture(this, true);
    }

    disableTexture() {
        return globalThis.__atlasSetParticleEmitterUseTexture(this, false);
    }

    setPosition(position) {
        this.position = position;
        return globalThis.__atlasSetParticleEmitterPosition(this, position);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        return globalThis.__atlasMoveParticleEmitter(this, position);
    }

    getPosition() {
        return globalThis.__atlasGetParticleEmitterPosition(this);
    }

    setEmissionType(type) {
        this.emissionType = type;
        return globalThis.__atlasSetParticleEmitterEmissionType(this, type);
    }

    setDirection(direction) {
        this.direction = direction;
        return globalThis.__atlasSetParticleEmitterDirection(this, direction);
    }

    setSpawnRadius(radius) {
        this.spawnRadius = radius;
        return globalThis.__atlasSetParticleEmitterSpawnRadius(this, radius);
    }

    setSpawnRate(rate) {
        this.spawnRate = rate;
        return globalThis.__atlasSetParticleEmitterSpawnRate(this, rate);
    }

    setParticleSettings(settings) {
        this.settings = settings;
        return globalThis.__atlasSetParticleEmitterSettings(this, settings);
    }

    emitOnce() {
        return globalThis.__atlasParticleEmitterEmitOnce(this);
    }

    emitContinuous() {
        return globalThis.__atlasParticleEmitterEmitContinuous(this);
    }

    startEmission() {
        return globalThis.__atlasParticleEmitterStartEmission(this);
    }

    stopEmission() {
        return globalThis.__atlasParticleEmitterStopEmission(this);
    }

    emitBurst(count) {
        return globalThis.__atlasParticleEmitterEmitBurst(this, count);
    }

    setRotation() {
        return undefined;
    }

    setScale() {
        return undefined;
    }

    lookAt() {
        return undefined;
    }

    rotate() {
        return undefined;
    }

    scaleBy() {
        return undefined;
    }

    show() {
        return globalThis.__atlasShowParticleEmitter(this.id);
    }

    hide() {
        return globalThis.__atlasHideParticleEmitter(this.id);
    }
}
