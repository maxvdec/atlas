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
    export abstract class Component {
        abstract init(): void;
        abstract update(deltaTime: number): void;
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
