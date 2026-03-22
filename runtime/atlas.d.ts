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
