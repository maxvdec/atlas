import { Color, Position2d, Position3d } from "atlas/units";

export class Component {
    init() {}
    update(deltaTime) {}

    getParent(type) {
        const parent = globalThis.__atlasGetObjectById(this.parentId);
        if (type == null) {
            return parent;
        }
        return parent?.getComponent(type) ?? null;
    }

    getObject(identifier) {
        if (typeof identifier === "number") {
            return globalThis.__atlasGetObjectById(identifier);
        }
        if (typeof identifier === "string") {
            return globalThis.__atlasGetObjectByName(identifier);
        }
        return null;
    }

    getCamera() {
        return globalThis.__atlasGetCamera();
    }
}

export class Material {
    constructor() {
        this.albedo = Color.white();
        this.metallic = 0;
        this.roughness = 0.5;
        this.ao = 1;
        this.reflectivity = 0;
        this.emissiveColor = Color.black();
        this.emissiveIntensity = 0;
        this.normalMapStrength = 1;
        this.useNormalMap = true;
        this.transmittance = 0;
        this.ior = 1;
    }
}

export class CoreVertex {
    constructor(
        position = Position3d.zero(),
        color = Color.white(),
        textureCoord = Position2d.zero(),
        normal = Position3d.zero(),
        tangent = Position3d.zero(),
        bitangent = Position3d.zero(),
    ) {
        this.position = position;
        this.color = color;
        this.textureCoord = textureCoord;
        this.normal = normal;
        this.tangent = tangent;
        this.bitangent = bitangent;
    }
}

export class Instance {
    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasCommitInstance(this);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasCommitInstance(this);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasCommitInstance(this);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasCommitInstance(this);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasCommitInstance(this);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasCommitInstance(this);
    }

    equals(other) {
        return (
            this.position.is(other.position) &&
            this.rotation.is(other.rotation) &&
            this.scale.is(other.scale)
        );
    }
}

export class GameObject {
    constructor() {
        this.id = -1;
        this.components = [];
        this.position = Position3d.zero();
        this.rotation = Position3d.zero();
        this.scale = new Position3d(1, 1, 1);
        this.name = "";
    }

    attachTexture(texture) {}

    setPosition(position) {
        this.position = position;
        globalThis.__atlasUpdateObject(this);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasUpdateObject(this);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasUpdateObject(this);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtObject(this, target, up);
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

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasUpdateObject(this);
    }

    show() {
        globalThis.__atlasShowObject(this.id);
    }

    hide() {
        globalThis.__atlasHideObject(this.id);
    }

    addComponent(component) {
        return globalThis.__atlasAddComponent(this.id, component);
    }
}

export class CoreObject extends GameObject {
    constructor() {
        super();
        this.vertices = [];
        this.indices = [];
        this.textures = [];
        this.material = new Material();
        this.instances = [];
        this.castsShadows = true;
        globalThis.__atlasCreateCoreObject(this);
    }

    makeEmissive(color, intensity) {
        globalThis.__atlasMakeEmissive(this.id, color, intensity);
    }

    attachVertices(vertices) {
        this.vertices = vertices;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    attachIndices(indices) {
        this.indices = indices;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setRotationQuaternion(rotation) {
        return globalThis.__atlasSetRotationQuaternion(this.id, rotation);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtObject(this, target, up);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    clone() {
        return globalThis.__atlasCloneCoreObject(this);
    }

    enableDeferredRendering() {
        globalThis.__atlasEnableDeferredRendering(this.id);
    }

    disableDeferredRendering() {
        globalThis.__atlasDisableDeferredRendering(this.id);
    }

    createInstance() {
        return globalThis.__atlasCreateInstance(this.id);
    }

    getComponent(type) {
        if (type == null) {
            return null;
        }
        return globalThis.__atlasGetComponentByName(this.id, type.name);
    }

    static box(size) {
        return globalThis.__atlasCreateBox(size);
    }

    static plane(size) {
        return globalThis.__atlasCreatePlane(size);
    }

    static pyramid(size) {
        return globalThis.__atlasCreatePyramid(size);
    }

    static sphere(radius, sectorCount = 36, stackCount = 18) {
        return globalThis.__atlasCreateSphere(radius, sectorCount, stackCount);
    }
}

export const ResourceType = Object.freeze({
    File: 0,
    Texture: 1,
    SpecularMap: 2,
    Audio: 3,
    Font: 4,
    Model: 5,
});

export class Resource {
    constructor(type, path, name) {
        this.type = type;
        this.path = path;
        this.name = name;
    }

    static fromAssetPath(path, type, name) {
        return globalThis.__atlasLoadResource(path, type, name);
    }

    static fromName(name, type) {
        return globalThis.__atlasGetResourceByName(name, type);
    }
}

export class ResourceGroup {
    constructor(resources, name) {
        this.resources = resources;
        this.name = name;
    }

    addResource(resource) {
        this.resources.push(resource);
    }

    getResourceByName(name) {
        return this.resources.find((r) => r.name === name) || null;
    }
}

export class Camera {
    constructor() {
        this.position = new Position3d(0, 0, 3);
        this.target = Position3d.zero();
        this.fov = 45;
        this.nearClip = 0.5;
        this.farClip = 1000;
        this.orthographicSize = 5;
        this.movementSpeed = 2;
        this.mouseSensitivity = 0.1;
        this.controllerLookSensitivity = 180;
        this.lookSmoothness = 0.15;
        this.useOrthographic = false;
        this.focusDepth = 20;
        this.focusRange = 10;
        return globalThis.__atlasGetCamera() ?? this;
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        return globalThis.__atlasUpdateCamera(this);
    }

    setPosition(position) {
        this.position = position;
        return globalThis.__atlasUpdateCamera(this);
    }

    setPositionKeepingOrientation(position) {
        return globalThis.__atlasSetPositionKeepingOrientation(this, position);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtCamera(this, target, up);
    }

    moveTo(position, speed) {
        return globalThis.__atlasMoveCameraTo(this, position, speed);
    }

    getDirection() {
        return globalThis.__atlasGetCameraDirection(this);
    }
}
