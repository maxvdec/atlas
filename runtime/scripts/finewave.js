import { Resource, ResourceType } from "atlas";

export class AudioEngine {
    constructor() {
        this.deviceName = "";
        return globalThis.__finewaveGetAudioEngine() ?? this;
    }

    setListenerPosition(position) {
        globalThis.__finewaveAudioEngineSetListenerPosition(this, position);
    }

    setListenerOrientation(forward, up) {
        globalThis.__finewaveAudioEngineSetListenerOrientation(
            this,
            forward,
            up,
        );
    }

    setListenerVelocity(velocity) {
        globalThis.__finewaveAudioEngineSetListenerVelocity(this, velocity);
    }

    setMasterVolume(volume) {
        globalThis.__finewaveAudioEngineSetMasterVolume(this, volume);
    }
}

export class AudioData {
    constructor() {
        this.isMono = false;
        this.resource = new Resource(ResourceType.Audio, "", "");
    }

    static fromResource(resource) {
        return globalThis.__finewaveCreateAudioData(resource);
    }
}

export class AudioSource {
    constructor() {
        return globalThis.__finewaveCreateAudioSource() ?? this;
    }

    setData(data) {
        globalThis.__finewaveAudioSourceSetData(this, data);
    }

    fromFile(resource) {
        globalThis.__finewaveAudioSourceFromFile(this, resource);
    }

    play() {
        globalThis.__finewaveAudioSourcePlay(this);
    }

    pause() {
        globalThis.__finewaveAudioSourcePause(this);
    }

    stop() {
        globalThis.__finewaveAudioSourceStop(this);
    }

    setLoop(loop) {
        globalThis.__finewaveAudioSourceSetLoop(this, loop);
    }

    setVolume(volume) {
        globalThis.__finewaveAudioSourceSetVolume(this, volume);
    }

    setPitch(pitch) {
        globalThis.__finewaveAudioSourceSetPitch(this, pitch);
    }

    setPosition(position) {
        globalThis.__finewaveAudioSourceSetPosition(this, position);
    }

    setVelocity(velocity) {
        globalThis.__finewaveAudioSourceSetVelocity(this, velocity);
    }

    isPlaying() {
        return globalThis.__finewaveAudioSourceIsPlaying(this);
    }

    playFrom(position) {
        globalThis.__finewaveAudioSourcePlayFrom(this, position);
    }

    disableSpatialization() {
        globalThis.__finewaveAudioSourceDisableSpatialization(this);
    }

    applyEffect(effect) {
        globalThis.__finewaveAudioSourceApplyEffect(this, effect);
    }

    getPosition() {
        return globalThis.__finewaveAudioSourceGetPosition(this);
    }

    getListenerPosition() {
        return globalThis.__finewaveAudioSourceGetListenerPosition(this);
    }

    useSpatialization() {
        globalThis.__finewaveAudioSourceUseSpatialization(this);
    }
}

export class AudioEffect {}

export class Reverb extends AudioEffect {
    constructor() {
        super();
        return globalThis.__finewaveCreateReverb() ?? this;
    }

    setRoomSize(size) {
        globalThis.__finewaveReverbSetRoomSize(this, size);
    }

    setDamping(damping) {
        globalThis.__finewaveReverbSetDamping(this, damping);
    }

    setWetLevel(level) {
        globalThis.__finewaveReverbSetWetLevel(this, level);
    }

    setDryLevel(level) {
        globalThis.__finewaveReverbSetDryLevel(this, level);
    }

    setWidth(width) {
        globalThis.__finewaveReverbSetWidth(this, width);
    }
}

export class Echo extends AudioEffect {
    constructor() {
        super();
        return globalThis.__finewaveCreateEcho() ?? this;
    }

    setDelay(delay) {
        globalThis.__finewaveEchoSetDelay(this, delay);
    }

    setDecay(decay) {
        globalThis.__finewaveEchoSetDecay(this, decay);
    }

    setWetLevel(level) {
        globalThis.__finewaveEchoSetWetLevel(this, level);
    }

    setDryLevel(level) {
        globalThis.__finewaveEchoSetDryLevel(this, level);
    }
}

export class Distortion extends AudioEffect {
    constructor() {
        super();
        return globalThis.__finewaveCreateDistortion() ?? this;
    }

    setEdge(edge) {
        globalThis.__finewaveDistortionSetEdge(this, edge);
    }

    setGain(gain) {
        globalThis.__finewaveDistortionSetGain(this, gain);
    }

    setLowpassCutoff(cutoff) {
        globalThis.__finewaveDistortionSetLowpassCutoff(this, cutoff);
    }
}
