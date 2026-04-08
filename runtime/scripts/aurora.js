import { GameObject, Resource, ResourceType } from "atlas";
import { Texture } from "atlas/graphics";
import { Color, Position3d } from "atlas/units";

function nextSeed() {
    const seed = Math.floor(Date.now() % 2147483647);
    Noise.seed = seed;
    Noise.initializedSeed = true;
    return seed;
}

function activeSeed() {
    if (!Noise.initializedSeed) {
        return nextSeed();
    }
    return Noise.seed;
}

export class PerlinNoise {
    constructor(seed = 0) {
        this.seed = seed;
    }

    noise(x, y) {
        return globalThis.__auroraPerlinNoise(this.seed, x, y);
    }
}

export class SimplexNoise {
    static noise(xin, yin) {
        return globalThis.__auroraSimplexNoise(xin, yin);
    }
}

export class WorleyNoise {
    constructor(numPoints, seed = 0) {
        this.numPoints = numPoints;
        this.seed = seed;
    }

    noise(x, y) {
        return globalThis.__auroraWorleyNoise(this.numPoints, this.seed, x, y);
    }
}

export class FractalNoise {
    constructor(o, p) {
        this.octaves = o;
        this.persistence = p;
    }

    noise(x, y) {
        return globalThis.__auroraFractalNoise(
            this.octaves,
            this.persistence,
            x,
            y,
        );
    }
}

export class Noise {
    static seed = 0;
    static initializedSeed = false;

    static perlin(x, y) {
        return new PerlinNoise(activeSeed()).noise(x, y);
    }

    static simplex(x, y) {
        activeSeed();
        return SimplexNoise.noise(x, y);
    }

    static worley(x, y) {
        return new WorleyNoise(10, activeSeed()).noise(x, y);
    }

    static fractal(x, y, octaves, persistence) {
        return new FractalNoise(octaves, persistence).noise(x, y);
    }
}

export class Biome {
    constructor(
        name = "",
        texture = null,
        color = Color.white(),
        useTexture = false,
    ) {
        this.name = name;
        this.texture = texture;
        this.color = color;
        this.useTexture = useTexture;
        this.minHeight = -1;
        this.maxHeight = -1;
        this.minMoisture = -1;
        this.maxMoisture = -1;
        this.minTemperature = -1;
        this.maxTemperature = -1;
        this.condition = () => {};
    }

    attachTexture(texture) {
        this.texture = texture;
        this.useTexture = true;
    }
}

export class Terrain extends GameObject {
    constructor(source = null, createdWithMap = false) {
        super();
        this.heightmap =
            createdWithMap && source instanceof Resource
                ? source
                : new Resource(ResourceType.Texture, "", "");
        this.moistureTexture = null;
        this.temperatureTexture = null;
        this.generator = createdWithMap ? null : source;
        this.createdWithMap = createdWithMap;
        this.width = 512;
        this.height = 512;
        this.length = 512;
        this.biomes = [];
        this.maxPeak = 48;
        this.seaLevel = 16;
        this.resolution = 20;
        return globalThis.__auroraCreateTerrain(this) ?? this;
    }

    _sync() {
        this.height = this.length ?? this.height;
        this.length = this.height;
        return globalThis.__auroraUpdateTerrain(this);
    }

    attachTexture(texture) {
        if (!(texture instanceof Texture)) {
            return;
        }
        if (this.biomes.length === 0) {
            this.biomes.push(new Biome("Default", texture, Color.white(), true));
        } else {
            this.biomes[0].attachTexture(texture);
        }
        this._sync();
    }

    setPosition(position) {
        this.position = position;
        this._sync();
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        this._sync();
    }

    setRotation(rotation) {
        this.rotation = rotation;
        this._sync();
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtObject(this, target, up);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        this._sync();
    }

    setScale(scale) {
        this.scale = scale;
        this._sync();
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        this._sync();
    }

    show() {
        globalThis.__atlasShowObject(this.id);
    }

    hide() {
        globalThis.__atlasHideObject(this.id);
    }

    addBiome(biome) {
        if (biome == null) {
            return;
        }
        if (typeof biome.condition === "function") {
            biome.condition(biome);
        }
        this.biomes.push(biome);
        this._sync();
    }

    static fromGenerator(generator) {
        return new Terrain(generator, false);
    }

    static fromHeightmap(heightmap) {
        const resource =
            heightmap instanceof Resource
                ? heightmap
                : Resource.fromAssetPath(
                      heightmap,
                      ResourceType.Texture,
                      "terrain-heightmap",
                  );
        return new Terrain(resource, true);
    }
}

export class TerrainGenerator {
    constructor() {
        this.algorithm = "generator";
        this.type = "generator";
    }

    generateHeight(x, y) {
        return 0;
    }

    applyTo(terrain) {
        if (!(terrain instanceof Terrain)) {
            return;
        }
        terrain.generator = this;
        terrain.createdWithMap = false;
        terrain._sync();
    }
}

export class HillGenerator extends TerrainGenerator {
    constructor(scale, amplitude) {
        super();
        this.algorithm = "hill";
        this.type = "hill";
        this.scale = scale;
        this.amplitude = amplitude;
    }

    generateHeight(x, y) {
        const noise = Noise.perlin(x / this.scale, y / this.scale);
        return ((noise + 1) * 0.5 * this.amplitude) / 10;
    }
}

export class MountainGenerator extends TerrainGenerator {
    constructor(scale, amplitude, octaves, persistance) {
        super();
        this.algorithm = "mountain";
        this.type = "mountain";
        this.scale = scale;
        this.amplitude = amplitude;
        this.octaves = octaves;
        this.persistance = persistance;
        this.persistence = persistance;
    }

    generateHeight(x, y) {
        const noise = Noise.fractal(
            x * this.scale,
            y * this.scale,
            this.octaves,
            this.persistence,
        );
        return noise * this.amplitude;
    }
}

export class PlainGenerator extends TerrainGenerator {
    constructor(scale, amplitude) {
        super();
        this.algorithm = "plain";
        this.type = "plain";
        this.scale = scale;
        this.amplitude = amplitude;
    }

    generateHeight(x, y) {
        const noise = Noise.perlin(x * this.scale, y * this.scale);
        return ((noise + 1) * 0.5 * this.amplitude) / 2;
    }
}

export class IslandGenerator extends TerrainGenerator {
    constructor(numFeatures, scale) {
        super();
        this.algorithm = "island";
        this.type = "island";
        this.numFeatures = numFeatures;
        this.scale = scale;
    }

    generateHeight(x, y) {
        const noise = new WorleyNoise(this.numFeatures, activeSeed()).noise(
            x * this.scale,
            y * this.scale,
        );
        return Math.max(0, Math.min(noise, 1));
    }
}

export class CompoundGenerator extends TerrainGenerator {
    constructor() {
        super();
        this.algorithm = "compound";
        this.type = "compound";
        this.generators = [];
    }

    addGenerator(generator) {
        if (generator != null) {
            this.generators.push(generator);
        }
    }

    generateHeight(x, y) {
        let height = 0;
        for (const generator of this.generators) {
            height += generator.generateHeight(x, y);
        }
        return height;
    }
}
