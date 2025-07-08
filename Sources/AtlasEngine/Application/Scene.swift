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
    
    func ensureLightBuffer() {
        if lightBuffer == nil {
            makeLightBuffer()
        }
    }

    func makeLightBuffer() {
        let device = RenderDispatcher.shared.device!
        if lights.count > 0 {
            var metalLights: [MetalLight] = []
            for light in lights {
                metalLights.append(light.toMetalLight())
            }

            lightBuffer = device.makeBuffer(bytes: metalLights, length: MemoryLayout<MetalLight>.stride * metalLights.count, options: [])
        } else {
            let dummyLight: [MetalLight] = [
                MetalLight(type: 0, color: .init(), position: .init(), intensity: 0.0)
            ]
            lightBuffer = device.makeBuffer(bytes: dummyLight, length: MemoryLayout<MetalLight>.stride, options: [])
        }
    }
}
