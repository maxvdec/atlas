//
//  Light.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

public enum LightType: UInt32 {
    case pointLight = 0
}

struct MetalLight {
    var type: UInt32
    var color: SIMD4<Float>
    var position: SIMD4<Float>
    var intensity: Float

    var specular: SIMD4<Float>
    var diffuse: SIMD4<Float>
}

public class Light {
    public var type: LightType = .pointLight
    public var color: Color = .shadeOfWhite(1)
    public var intensity: Float = 1.0
    public var position: Position3d
    public var material: Material = .init()

    var debugObject: CoreObject?

    public init(position: Position3d, type: LightType, color: Color = .shadeOfWhite(1), intensity: Float = 1.0) {
        self.type = type
        self.color = color
        self.intensity = intensity
        self.position = position
        material.diffuse = [0.5, 0.5, 0.5]
    }

    func toMetalLight() -> MetalLight {
        return MetalLight(type: type.rawValue, color: color.toSimd(), position: position.toSimd4(), intensity: intensity, specular: material.specular.toSimd(), diffuse: material.diffuse.toSimd())
    }

    public func addToScene() {
        RenderDispatcher.shared.currentScene.addLight(self)
    }

    public func debugLight() {
        if debugObject == nil {
            debugObject = generateCubeObject(size: [0.1, 0.1, 0.1])
            debugObject!.shader = BasicShader()
            debugObject!.move(by: position)
            debugObject!.initialize()
        }
        debugObject!.isRendering = true
    }

    public func hideDebugLight() {
        if debugObject == nil {
            debugObject = generateCubeObject(size: [0.1, 0.1, 0.1])
            debugObject!.shader = BasicShader()
            debugObject!.move(by: position)
            debugObject!.initialize()
        }
        debugObject!.isRendering = false
    }
}
