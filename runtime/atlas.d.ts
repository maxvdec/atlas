//
// atlas.d.ts
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Declarations for the Atlas Library for scripting
// Copyright (c) 2026 Max Van den Eynde
//

declare module "atlas/log" {
    export const Debug: {
        print(message: string): void;
        warning(message: string): void;
        error(message: string): void;
    };
}

declare module "atlas" {
    import {
        Position3d,
        Rotation3d,
        Scale3d,
        Color,
        Position2d,
        Size2d,
        Quaternion,
        Normal3d,
        Size3d,
        Point3d,
    } from "atlas/units";
    import { Texture, Skybox } from "atlas/graphics";

    export type Fog = {
        color: Color;
        intensity: number;
    };

    export type VolumetricLighting = {
        enabled: boolean;
        density: number;
        weight: number;
        decay: number;
        exposure: number;
    };

    export type LightBloomConfiguration = {
        radius: number;
        maxSamples: number;
    };

    export type RimLightingConfiguration = {
        color: Color;
        intensity: number;
    };

    export type Environment = {
        fog: Fog;
        volumetricLighting: VolumetricLighting;
        lightBloom: LightBloomConfiguration;
        rimLighting: RimLightingConfiguration;
        lookupTexture: Texture;
    };

    export class Scene {
        name: string;

        setAmbientIntensity(intensity: number): void;
        setAutomaticAmbient(enabled: boolean): void;

        setSkybox(skybox: Skybox): void;
        useAtmosphereSkybox(enabled: boolean): void;

        getCamera(): Camera;

        //atmosphere: Atmosphere;
    }

    export abstract class Component {
        parentId: number;

        abstract init(): void;
        abstract update(deltaTime: number): void;

        getParent(): GameObject;
        getParent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;
        getObject(identifier: number | string): CoreObject;
        getScene(): Scene;
    }

    export class Material {
        constructor();

        albedo: Color;
        metallic: number;
        roughness: number;
        ao: number;
        reflectivity: number;
        emissiveColor: Color;
        emissiveIntensity: number;
        normalMapStrength: number;
        useNormalMap: boolean;
        transmittance: number;
        ior: number;
    }

    export class CoreVertex {
        constructor(
            position?: Position3d,
            color?: Color,
            textureCoord?: Position2d,
            normal?: Normal3d,
            tangent?: Normal3d,
            bitangent?: Normal3d,
        );

        position: Position3d;
        color: Color;
        textureCoord: Position2d;
        normal: Normal3d;
        tangent: Normal3d;
        bitangent: Normal3d;
    }

    export class Instance {
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;

        move(position: Position3d): void;
        setPosition(position: Position3d): void;
        setRotation(rotation: Rotation3d): void;
        rotate(rotation: Rotation3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;

        equals(other: Instance): boolean;
    }

    export abstract class GameObject {
        id: number;
        components: Component[];
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;
        name: string;

        constructor();

        abstract attachTexture(texture: Texture): void;
        abstract setPosition(position: Position3d): void;
        abstract move(position: Position3d): void;
        abstract setRotation(rotation: Rotation3d): void;
        abstract lookAt(target: Position3d, up?: Normal3d): void;
        abstract rotate(rotation: Rotation3d): void;
        abstract setScale(scale: Scale3d): void;
        abstract scaleBy(scale: Scale3d): void;
        abstract show(): void;
        abstract hide(): void;

        addComponent<T extends Component>(component: T): void;
    }

    export class CoreObject extends GameObject {
        vertices: CoreVertex[];
        indices: number[];
        textures: Texture[];
        material: Material;
        instances: Instance[];
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;
        castsShadows: boolean;
        name: string;

        constructor();

        makeEmissive(color: Color, intensity: number): void;
        attachVertices(vertices: CoreVertex[]): void;
        attachIndices(indices: number[]): void;
        attachTexture(texture: Texture): void;

        setPosition(position: Position3d): void;
        move(position: Position3d): void;
        setRotation(rotation: Rotation3d): void;
        setRotationQuaternion(rotation: Quaternion): void;
        rotate(rotation: Rotation3d): void;
        lookAt(target: Position3d, up?: Normal3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;

        clone(): CoreObject;

        show(): void;
        hide(): void;

        addComponent<T extends Component>(component: T): void;

        enableDeferredRendering(): void;
        disableDeferredRendering(): void;

        createInstance(): Instance;

        getComponent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;

        static box(size: Size3d): CoreObject;
        static plane(size: Size2d): CoreObject;
        static pyramid(size: Size3d): CoreObject;
        static sphere(
            radius: number,
            sectorCount: number,
            stackCount: number,
        ): CoreObject;
    }

    export enum ResourceType {
        File,
        Texture,
        SpecularMap,
        Audio,
        Font,
        Model,
    }

    export class Resource {
        type: ResourceType;
        path: string;
        name: string;

        constructor(type: ResourceType, path: string, name: string);

        static fromAssetPath(
            path: string,
            type: ResourceType,
            name?: string,
        ): Resource;

        static fromName(name: string, type: ResourceType): Resource | null;
    }

    export class ResourceGroup {
        resources: Resource[];
        name: string;

        constructor(resources: Resource[], name: string);

        addResource(resource: Resource): void;
        getResourceByName(name: string): Resource | null;
    }

    export class Camera {
        position: Position3d;
        target: Point3d;
        fov: number;
        nearClip: number;
        farClip: number;
        orthographicSize: number;
        movementSpeed: number;
        mouseSensitivity: number;
        controllerLookSensitivity: number;
        lookSmoothness: number;
        useOrthographic: boolean;
        focusDepth: number;
        focusRange: number;

        constructor();

        move(offset: Position3d): void;
        setPosition(position: Position3d): void;
        setPositionKeepingOrientation(position: Position3d): void;
        lookAt(target: Point3d, up?: Normal3d): void;
        moveTo(target: Point3d, speed: number): void;
        getDirection(): Normal3d;
    }
}

declare module "atlas/graphics" {
    import { Resource, ResourceGroup } from "atlas";
    import { Color, Magnitude2d } from "atlas/units";

    export enum TextureType {
        Color,
        Specular,
        Cubemap,
        Depth,
        DepthCube,
        Normal,
        Parallax,
        SSAONoise,
        SSAO,
        Metallic,
        Roughness,
        AO,
        Opacity,
        HDR,
    }

    export class Texture {
        type: TextureType;
        resource: Resource;
        width: number;
        height: number;
        channels: number;
        id: number;
        borderColor: Color;

        static fromResource(
            resource: Resource | string,
            type: TextureType,
        ): Texture;

        static createEmpty(
            width: number,
            height: number,
            type: TextureType,
            borderColor?: Color,
        ): Texture;

        static createColor(
            color: Color,
            type: TextureType,
            width: number,
            height: number,
        ): Texture;

        createCheckerboard(
            width: number,
            height: number,
            checkSize: number,
            color1: Color,
            color2: Color,
        ): void;

        createDoubleCheckerboard(
            width: number,
            height: number,
            checkSizeBig: number,
            checkSizeSmall: number,
            color1: Color,
            color2: Color,
            color3: Color,
        ): void;

        displayToWindow(): void;
    }

    export class Cubemap {
        resources: Resource[];
        id: number;

        constructor(resources: Resource[]);

        getAverageColor(): Color;
        static fromResourceGroup(resourceGroup: ResourceGroup): Cubemap | null;
        updateWithColors(colors: Color[]): void;
    }

    export enum RenderTargetType {
        Scene,
        Multisampled,
        Shadow,
        CubeShadow,
        GBuffer,
        SSAO,
        SSAOBlur,
    }

    export enum RenderPassType {
        Deferred,
        Forward,
        PathTracing,
    }

    export const Effects: {
        Inversion: { type: "Inversion" };
        Grayscale: { type: "Grayscale" };
        Sharpen: { type: "Sharpen" };
        Blur: { type: "Blur"; magnitude: number };
        EdgeDetection: { type: "EdgeDetection" };
        ColorCorrection: {
            type: "ColorCorrection";
            exposure: number;
            contrast: number;
            saturation: number;
            gamma: number;
            temperature: number;
            tint: number;
        };
        MotionBlur: { type: "MotionBlur"; size: number; separation: number };
        ChromaticAberration: {
            type: "ChromaticAberration";
            red: number;
            green: number;
            blue: number;
            direction: Magnitude2d;
        };
        Posterization: { type: "Posterization"; levels: number };
        Pixelation: { type: "Pixelation"; pixelSize: number };
        Dialation: { type: "Dilation"; size: number; separation: number };
        Dilation: { type: "Dilation"; size: number; separation: number };
        FilmGrain: { type: "FilmGrain"; amount: number };
    };

    export type Effect =
        | keyof typeof Effects
        | (typeof Effects)[keyof typeof Effects];

    export class RenderTarget {
        type: RenderTargetType;
        resolution: number;
        outTextures: Texture[];
        depthTexture: Texture | null;

        constructor(type: RenderTargetType, resolution: number);

        addEffect(effect: Effect): void;

        addToPassQueue(type: RenderPassType): void;
        display(): void;
    }

    export class Skybox {
        cubemap: Cubemap;

        constructor(cubemap: Cubemap);
    }
}

declare module "atlas/units" {
    export class Position3d {
        x: number;
        y: number;
        z: number;

        constructor(x: number, y: number, z: number);

        static zero(): Position3d;
        static down(): Position3d;
        static up(): Position3d;
        static forward(): Position3d;
        static back(): Position3d;
        static right(): Position3d;
        static left(): Position3d;
        static invalid(): Position3d;

        add(other: Position3d | number): Position3d;
        subtract(other: Position3d | number): Position3d;
        multiply(other: Position3d | number): Position3d;
        divide(other: Position3d | number): Position3d;
        is(other: Position3d): boolean;

        normalized(): Position3d;
        toString(): string;
    }

    export type Scale3d = Position3d;
    export type Size3d = Position3d;
    export type Point3d = Position3d;
    export type Normal3d = Position3d;
    export type Magnitude3d = Position3d;
    export type Impulse3d = Position3d;
    export type Force3d = Position3d;
    export type Vector3d = Position3d;
    export type Velocity3d = Position3d;
    export type Rotation3d = Position3d;

    export class BoundingBox {
        min: Position3d;
        max: Position3d;

        constructor(min: Position3d, max: Position3d);

        toString(): string;
        contains(point: Position3d): boolean;
        intersects(other: BoundingBox): boolean;
    }

    export class Quaternion {
        x: number;
        y: number;
        z: number;
        w: number;

        constructor(x: number, y: number, z: number, w: number);
        constructor(rotation: Rotation3d);

        toEuler(): Rotation3d;
        static fromEuler(rotation: Rotation3d): Quaternion;
    }

    export class Color {
        r: number;
        g: number;
        b: number;
        a: number;

        constructor(r: number, g: number, b: number, a?: number);

        add(other: Color | number): Color;
        subtract(other: Color | number): Color;
        multiply(other: Color | number): Color;
        divide(other: Color | number): Color;
        is(other: Color): boolean;

        static white(): Color;
        static black(): Color;
        static red(): Color;
        static green(): Color;
        static blue(): Color;
        static transparent(): Color;
        static yellow(): Color;
        static cyan(): Color;
        static magenta(): Color;
        static gray(): Color;
        static orange(): Color;
        static purple(): Color;
        static brown(): Color;
        static pink(): Color;
        static lime(): Color;
        static navy(): Color;
        static teal(): Color;
        static olive(): Color;
        static maroon(): Color;

        static fromHex(hex: string): Color;
        static mix(color1: Color, color2: Color, t: number): Color;
    }

    export enum Direction3d {
        Up,
        Down,
        Left,
        Right,
        Forward,
        Backward,
    }

    export class Position2d {
        x: number;
        y: number;

        constructor(x: number, y: number);

        static zero(): Position2d;
        static up(): Position2d;
        static down(): Position2d;
        static left(): Position2d;
        static right(): Position2d;
        static invalid(): Position2d;

        add(other: Position2d | number): Position2d;
        subtract(other: Position2d | number): Position2d;
        multiply(other: Position2d | number): Position2d;
        divide(other: Position2d | number): Position2d;

        is(other: Position2d): boolean;
    }

    export type Scale2d = Position2d;
    export type Point2d = Position2d;
    export type Movement2d = Position2d;
    export type Magnitude2d = Position2d;

    export class Radians {
        value: number;

        constructor(value: number);

        add(other: Radians): Radians;
        subtract(other: Radians): Radians;
        multiply(other: Radians | number): Radians;
        divide(other: Radians | number): Radians;

        toNumber(): number;
        static fromDegrees(degrees: number): Radians;
        toDegrees(): number;
    }

    export class Size2d {
        width: number;
        height: number;

        constructor(width: number, height: number);

        static zero(): Size2d;

        toString(): string;

        add(other: Size2d | number): Size2d;
        subtract(other: Size2d | number): Size2d;
        multiply(other: Size2d | number): Size2d;
        divide(other: Size2d | number): Size2d;

        is(other: Size2d): boolean;
    }
}

declare module "atlas/audio" {
    import { Component, Resource } from "atlas";
    import { Color, Position3d } from "atlas/units";

    export class AudioPlayer extends Component {
        constructor();

        override init(): void;
        play(): void;
        pause(): void;
        stop(): void;
        setVolume(volume: number): void;
        setLoop(loop: boolean): void;

        setSource(resource: Resource): void;

        override update(dt: number): void;

        setPosition(position: Position3d): void;
        useSpatialAudio(enabled: boolean): void;
    }
}

declare module "atlas/input" {
    import { Position2d } from "atlas/units";

    export enum Key {
        Unknown,
        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Key0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,
        Semicolon,
        Equal,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        KP0,
        KP1,
        KP2,
        KP3,
        KP4,
        KP5,
        KP6,
        KP7,
        KP8,
        KP9,
        KPDecimal,
        KPDivide,
        KPMultiply,
        KPSubtract,
        KPAdd,
        KPEnter,
        KPEqual,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu,
    }

    export enum MouseButton {
        Left,
        Right,
        Middle,
        X1,
        X2,
        Button6,
        Button7,
        Button8,
        Last,
    }

    export enum TriggerType {
        MouseButton,
        Key,
        ControllerButton,
    }

    export type ControllerButtonTrigger = {
        controllerID: number;
        buttonIndex: number;
    };

    export class Trigger {
        type: TriggerType;
        mouseButton?: MouseButton;
        key?: Key;
        controllerButton?: ControllerButtonTrigger;

        static fromKey(key: Key): Trigger;
        static fromMouseButton(mouseButton: MouseButton): Trigger;
        static fromControllerButton(
            controllerID: number,
            buttonIndex: number,
        ): Trigger;
    }

    export enum AxisTriggerType {
        MouseAxis,
        KeyCustom,
        ControllerAxis,
    }

    export class AxisTrigger {
        type: AxisTriggerType;

        positiveX: Trigger;
        negativeX: Trigger;
        positiveY: Trigger;
        negativeY: Trigger;

        controllerId?: number;
        controllerAxisSingle: boolean;
        axisIndex?: number;
        axisIndexY: number;

        isJoystick: boolean;

        static fromMouse(): AxisTrigger;
        static fromKeys(
            positiveX: Key,
            negativeX: Key,
            positiveY: Key,
            negativeY: Key,
        ): AxisTrigger;
        static fromControllerAxis(
            controllerId: number,
            axisIndex: number,
            single: boolean,
            axisIndexY?: number,
        ): AxisTrigger;
    }

    export type AxisPacket = {
        deltaX: number;
        deltaY: number;
        x: number;
        y: number;
        valueX: number;
        valueY: number;
        inputDeltaX: number;
        inputDeltaY: number;
        hasValueInput: boolean;
        hasDeltaInput: boolean;
    };

    export type MousePacket = {
        xpos: number;
        ypos: number;
        xoffset: number;
        yoffset: number;
        constrainPitch: boolean;
        firstMouse: boolean;
    };

    export type MouseScrollPacket = {
        xoffset: number;
        yoffset: number;
    };

    export class InputAction {
        triggers: Trigger[];
        axisTriggers: AxisTrigger[];
        name: string;
        isAxis: boolean;
        isAxisSingle: boolean;
        normalized: boolean;
        invertY: boolean;

        static createButtonAction(
            name: string,
            triggers: Trigger[],
        ): InputAction;
        static createAxisAction(
            name: string,
            axisTriggers: AxisTrigger[],
        ): InputAction;
        static createSingleAxisAction(
            name: string,
            positiveTrigger: Trigger,
            negativeTrigger: Trigger,
        ): InputAction;
    }

    export const Input: {
        addAction(action: InputAction): InputAction;
        resetActions(): void;

        isKeyActive(key: Key): boolean;
        isKeyPressed(key: Key): boolean;
        isMouseButtonActive(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;

        getTextInput(): string;
        startTextInput(): void;
        stopTextInput(): void;
        isTextInputActive(): boolean;

        isControllerButtonPressed(
            controllerID: number,
            buttonIndex: number,
        ): boolean;
        getControllerAxisValue(
            controllerID: number,
            axisIndex: number,
        ): number;
        getControllerAxisPairValue(
            controllerID: number,
            axisIndexX: number,
            axisIndexY: number,
        ): Position2d;

        captureMouse(): void;
        releaseMouse(): void;
        getMousePosition(): Position2d;

        isActionTriggered(name: string): boolean;
        isActionCurrentlyActive(name: string): boolean;
        getAxisActionValue(name: string): AxisPacket;
    };

    export abstract class Interactive {
        abstract onKeyPress(key: Key, dt: number): void;
        abstract onKeyRelease(key: Key, dt: number): void;
        abstract onMouseMove(packet: MousePacket, dt: number): void;
        abstract onMouseButtonPress(button: MouseButton, dt: number): void;
        abstract onMouseScroll(packet: MouseScrollPacket, dt: number): void;
        abstract onEachFrame(dt: number): void;
    }
}
