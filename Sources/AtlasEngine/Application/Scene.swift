//
//  Scene.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import Metal
import MetalKit

public class RenderScene {
    public var ambientColor: Color = [0.25, 0.25, 0.25]
    public var lights: [Light] = [] {
        didSet {
            makeLightBuffer()
        }
    }

    var lightBuffer: MTLBuffer?

    public init() {}

    public func setAsCurrent() {
        RenderDispatcher.shared.currentScene = self
    }

    public func addLight(_ light: Light) {
        lights.append(light)
    }

    func makeLightBuffer() {
        let device = RenderDispatcher.shared.device!
        var metalLights: [MetalLight] = []
        for light in lights {
            metalLights.append(light.toMetalLight())
        }

        lightBuffer = device.makeBuffer(bytes: metalLights, length: MemoryLayout<MetalLight>.stride * metalLights.count, options: [])
    }
}
