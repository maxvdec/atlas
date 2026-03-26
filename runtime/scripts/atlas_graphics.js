import { Resource, ResourceType } from "atlas";
import { Color } from "atlas/units";

export const TextureType = Object.freeze({
    Color: 0,
    Specular: 1,
    Cubemap: 2,
    Depth: 3,
    DepthCube: 4,
    Normal: 5,
    Parallax: 6,
    SSAONoise: 7,
    SSAO: 8,
    Metallic: 9,
    Roughness: 10,
    AO: 11,
    Opacity: 12,
    HDR: 13,
});

export const RenderTargetType = Object.freeze({
    Scene: 0,
    Multisampled: 1,
    Shadow: 2,
    CubeShadow: 3,
    GBuffer: 4,
    SSAO: 5,
    SSAOBlur: 6,
});

export const RenderPassType = Object.freeze({
    Deferred: 0,
    Forward: 1,
    PathTracing: 2,
});

export const Effects = Object.freeze({
    Inversion: { type: "Inversion" },
    Grayscale: { type: "Grayscale" },
    Sharpen: { type: "Sharpen" },
    Blur: { type: "Blur", magnitude: 16 },
    EdgeDetection: { type: "EdgeDetection" },
    ColorCorrection: {
        type: "ColorCorrection",
        exposure: 0,
        contrast: 1,
        saturation: 1,
        gamma: 1,
        temperature: 0,
        tint: 0,
    },
    MotionBlur: { type: "MotionBlur", size: 8, separation: 1 },
    ChromaticAberration: {
        type: "ChromaticAberration",
        red: 0.01,
        green: 0.006,
        blue: -0.006,
        direction: { x: 0, y: 0 },
    },
    Posterization: { type: "Posterization", levels: 5 },
    Pixelation: { type: "Pixelation", pixelSize: 8 },
    Dialation: { type: "Dilation", size: 8, separation: 1 },
    Dilation: { type: "Dilation", size: 8, separation: 1 },
    FilmGrain: { type: "FilmGrain", amount: 0.05 },
});

export class Texture {
    constructor() {
        this.type = TextureType.Color;
        this.resource = new Resource(ResourceType.File, "", "");
        this.width = 0;
        this.height = 0;
        this.channels = 0;
        this.id = 0;
        this.borderColor = Color.black();
    }

    static fromResource(resource, type = TextureType.Color) {
        return globalThis.__atlasCreateTextureFromResource(resource, type);
    }

    static createEmpty(
        width,
        height,
        type = TextureType.Color,
        borderColor = new Color(0, 0, 0, 0),
    ) {
        return globalThis.__atlasCreateEmptyTexture(
            width,
            height,
            type,
            borderColor,
        );
    }

    static createColor(color, type = TextureType.Color, width = 1, height = 1) {
        return globalThis.__atlasCreateColorTexture(color, type, width, height);
    }

    createCheckerboard(width, height, checkSize, color1, color2) {
        return globalThis.__atlasCreateCheckerboardTexture(
            this,
            width,
            height,
            checkSize,
            color1,
            color2,
        );
    }

    createDoubleCheckerboard(
        width,
        height,
        checkSizeBig,
        checkSizeSmall,
        color1,
        color2,
        color3,
    ) {
        return globalThis.__atlasCreateDoubleCheckerboardTexture(
            this,
            width,
            height,
            checkSizeBig,
            checkSizeSmall,
            color1,
            color2,
            color3,
        );
    }

    displayToWindow() {
        return globalThis.__atlasDisplayTexture(this);
    }
}

export class Cubemap {
    constructor(resources) {
        this.resources = resources;
        this.id = 0;
        return globalThis.__atlasCreateCubemap(resources);
    }

    getAverageColor() {
        return globalThis.__atlasGetCubemapAverageColor(this);
    }

    static fromResourceGroup(resourceGroup) {
        if (resourceGroup == null) {
            return null;
        }
        return globalThis.__atlasCreateCubemapFromGroup(resourceGroup.resources);
    }

    updateWithColors(colors) {
        return globalThis.__atlasUpdateCubemapWithColors(this, colors);
    }
}

export class RenderTarget {
    constructor(type = RenderTargetType.Scene, resolution = 1024) {
        this.type = type;
        this.resolution = resolution;
        this.outTextures = [];
        this.depthTexture = null;
        return globalThis.__atlasCreateRenderTarget(type, resolution);
    }

    addEffect(effect) {
        return globalThis.__atlasAddRenderTargetEffect(this, effect);
    }

    addToPassQueue(type) {
        return globalThis.__atlasAddRenderTargetToPassQueue(this, type);
    }

    addToPass(type) {
        return this.addToPassQueue(type);
    }

    display() {
        return globalThis.__atlasDisplayRenderTarget(this);
    }
}

export class Skybox {
    constructor(cubemap) {
        this.cubemap = cubemap;
        return globalThis.__atlasCreateSkybox(cubemap);
    }
}
