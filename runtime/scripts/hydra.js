import { GameObject } from "atlas";
import { Color, Position3d, Size2d } from "atlas/units";

export const WeatherCondition = Object.freeze({
    Clear: 0,
    Rain: 1,
    Snow: 2,
    Storm: 3,
});

function resolved(value, fallback) {
    return value == null ? fallback : value;
}

function syncClouds(clouds) {
    return resolved(globalThis.__hydraUpdateClouds(clouds), clouds);
}

function syncAtmosphere(atmosphere) {
    return resolved(globalThis.__hydraUpdateAtmosphere(atmosphere), atmosphere);
}

export class WorleyNoise3D {
    constructor(frequency, numDivisions) {
        return resolved(
            globalThis.__hydraCreateWorleyNoise(frequency, numDivisions),
            this,
        );
    }

    getValue(x, y, z) {
        return globalThis.__hydraWorleyGetValue(this, x, y, z);
    }

    get3dTexture(size) {
        return globalThis.__hydraWorleyGet3dTexture(this, size);
    }

    getDetailTexture(size) {
        return globalThis.__hydraWorleyGetDetailTexture(this, size);
    }

    get3dTextureAtAllChannels(size) {
        return globalThis.__hydraWorleyGetAllChannelsTexture(this, size);
    }
}

export class Clouds {
    constructor(frequency, numDivisions) {
        this.position = new Position3d(0, 5, 0);
        this.size = new Position3d(10, 3, 10);
        this.scale = 1.5;
        this.offset = Position3d.zero();
        this.density = 0.45;
        this.densityMultiplier = 1.5;
        this.absorption = 1.1;
        this.scattering = 0.85;
        this.phase = 0.55;
        this.clusterStrength = 0.5;
        this.primaryStepCount = 12;
        this.lightStepCount = 6;
        this.lightStepMultiplier = 1.6;
        this.minStepLength = 0.05;
        this.wind = new Position3d(0.02, 0, 0.01);
        return resolved(
            globalThis.__hydraCreateClouds(frequency, numDivisions),
            this,
        );
    }

    getCloudTexture(size) {
        syncClouds(this);
        return globalThis.__hydraCloudsGetTexture(this, size);
    }
}

export class Atmosphere {
    constructor() {
        this.timeOfDay = 12;
        this.secondsPerHour = 3600;
        this.wind = Position3d.zero();
        this.weatherDelegate = null;
        this.clouds = null;
        this.sunColor = new Color(1, 0.95, 0.8, 1);
        this.moonColor = new Color(0.5, 0.5, 0.8, 1);
        this.sunSize = 1;
        this.moonSize = 1;
        this.sunTintStrength = 0.3;
        this.moonTintStrength = 0.8;
        this.starIntensity = 3;
        this.cycle = false;
        return resolved(globalThis.__hydraCreateAtmosphere(), this);
    }

    enable() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereEnable(this);
    }

    disable() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereDisable(this);
    }

    isEnabled() {
        return globalThis.__hydraAtmosphereIsEnabled(this);
    }

    enableWeather() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereEnableWeather(this);
    }

    disableWeather() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereDisableWeather(this);
    }

    getNormalizedTime() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetNormalizedTime(this);
    }

    getSunAngle() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetSunAngle(this);
    }

    getMoonAngle() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetMoonAngle(this);
    }

    getLightIntensity() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetLightIntensity(this);
    }

    getLightColor() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetLightColor(this);
    }

    getSkyboxColors() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereGetSkyboxColors(this);
    }

    createSkyCubemap(size) {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereCreateSkyCubemap(this, size);
    }

    updateSkyCubemap(cubemap) {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereUpdateSkyCubemap(this, cubemap);
    }

    castShadowsFromSunlight(resolution) {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereCastShadows(this, resolution);
    }

    useGlobalLight() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereUseGlobalLight(this);
    }

    isDaytime() {
        syncAtmosphere(this);
        return globalThis.__hydraAtmosphereIsDaytime(this);
    }

    setTime(hours, minutes = 0, seconds = 0) {
        this.timeOfDay = hours + (minutes / 60) + (seconds / 3600);
        globalThis.__hydraAtmosphereSetTime(this, hours, minutes, seconds);
    }

    addClouds(frequency = 4, numDivisions = 6) {
        syncAtmosphere(this);
        const updated = resolved(
            globalThis.__hydraAtmosphereAddClouds(this, frequency, numDivisions),
            this,
        );
        this.clouds = updated.clouds;
    }

    resetRuntimeState() {
        syncAtmosphere(this);
        globalThis.__hydraAtmosphereResetRuntimeState(this);
    }
}

export class Fluid extends GameObject {
    constructor() {
        super();
        this.waveVelocity = 0;
        this.normalTexture = null;
        this.movementTexture = null;
        return resolved(globalThis.__hydraCreateFluid(this), this);
    }

    create(extent, color) {
        globalThis.__hydraFluidCreate(this, extent, color);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasUpdateObject(this);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasUpdateObject(this);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasUpdateObject(this);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasUpdateObject(this);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasUpdateObject(this);
    }

    setExtent(extent) {
        globalThis.__hydraFluidSetExtent(this, extent);
    }

    setWaveVelocity(velocity) {
        this.waveVelocity = velocity;
        globalThis.__hydraFluidSetWaveVelocity(this, velocity);
    }

    setWaterColor(color) {
        globalThis.__hydraFluidSetWaterColor(this, color);
    }

    getPosition() {
        return this.position;
    }

    getScale() {
        return this.scale;
    }
}
