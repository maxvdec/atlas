//
//  Scene.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import Metal
import MetalKit

public protocol SceneInteractive: Interactive {}

public extension SceneInteractive {
    func getObjects() -> [CoreObject] {
        return RenderDispatcher.shared.objects
    }

    func getPresents() -> [FullscreenPresent] {
        let objects = RenderDispatcher.shared.postObjects
        var fullScreen: [FullscreenPresent] = []
        for object in objects {
            if object is FullscreenPresent {
                fullScreen.append(object as! FullscreenPresent)
            }
        }
        return fullScreen
    }
}

public class RenderScene {
    public var ambientColor: Color = [0.25, 0.25, 0.25]
    public var lights: [Light] = [] {
        didSet {
            makeLightBuffer()
        }
    }

    var lightBuffer: MTLBuffer?

    public init(interactive: SceneInteractive? = nil) {
        if interactive != nil {
            RenderDispatcher.shared.registerFrameObject(interactive!)
        }
    }

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
                MetalLight(type: 0, color: .init(), position: .init(), direction: .init(), intensity: 0.0, specular: .init(), diffuse: .init(), constant: 0.0, linear: 0.0, quadratic: 0.0, innerCutoff: 0.0, outerCutoff: 0.0)
            ]
            lightBuffer = device.makeBuffer(bytes: dummyLight, length: MemoryLayout<MetalLight>.stride, options: [])
        }
    }
}
