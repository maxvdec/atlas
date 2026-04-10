import { Component } from "atlas";
import { Position3d } from "atlas/units";

export class AudioPlayer extends Component {
    constructor() {
        super();
        this.id = -1;
        this.source = null;
        this.volume = 1;
        this.loop = false;
        this.position = Position3d.zero();
        this.spatialAudio = false;
        globalThis.__atlasCreateAudioPlayer(this);
    }

    init() {
        globalThis.__atlasInitAudioPlayer(this.id);
    }

    play() {
        globalThis.__atlasPlayAudioPlayer(this.id);
    }

    pause() {
        globalThis.__atlasPauseAudioPlayer(this.id);
    }

    stop() {
        globalThis.__atlasStopAudioPlayer(this.id);
    }

    setVolume(volume) {
        this.volume = volume;
        globalThis.__atlasSetAudioPlayerVolume(this.id, volume);
    }

    setLoop(loop) {
        this.loop = loop;
        globalThis.__atlasSetAudioPlayerLoop(this.id, loop);
    }

    setSource(resource) {
        globalThis.__atlasSetAudioPlayerSource(this.id, resource);
    }

    update(dt) {
        globalThis.__atlasUpdateAudioPlayer(this.id, dt);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasSetAudioPlayerPosition(this.id, position);
    }

    useSpatialAudio(enabled) {
        this.spatialAudio = enabled;
        globalThis.__atlasUseSpatialAudio(this.id, enabled);
    }
}
