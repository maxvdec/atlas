//
//  Light.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import Foundation
import simd

public enum LightType: UInt32 {
    case pointLight = 0
    case directionalLight = 1
    case spotlight = 2
}

struct AttenuationProperties {
    var constant: Float
    var linear: Float
    var quadratic: Float
}

public enum LightDistance {
    case distance7
    case distance13
    case distance20
    case distance50
    case distance100
    case distance200

    func getProperties() -> AttenuationProperties {
        switch self {
        case .distance7:
            return AttenuationProperties(constant: 1.0, linear: 0.7, quadratic: 1.8)
        case .distance13:
            return AttenuationProperties(constant: 1.0, linear: 0.35, quadratic: 0.44)
        case .distance20:
            return AttenuationProperties(constant: 1.0, linear: 0.22, quadratic: 0.20)
        case .distance50:
            return AttenuationProperties(constant: 1.0, linear: 0.09, quadratic: 0.032)
        case .distance100:
            return AttenuationProperties(constant: 1.0, linear: 0.045, quadratic: 0.0075)
        case .distance200:
            return AttenuationProperties(constant: 1.0, linear: 0.022, quadratic: 0.0019)
        }
    }
}

struct MetalLight {
    var type: UInt32
    var color: SIMD4<Float>
    var position: SIMD4<Float>
    var direction: SIMD4<Float>
    var intensity: Float

    var specular: SIMD4<Float>
    var diffuse: SIMD4<Float>

    var constant: Float
    var linear: Float
    var quadratic: Float
    
    var innerCutoff: Float
    var outerCutoff: Float
}

public class Light {
    public var type: LightType = .pointLight
    public var color: Color = .shadeOfWhite(1)
    public var intensity: Float = 1.0
    public var position: Position3d = .init(x: 0, y: 0, z: 0)
    public var direction: Position3d = .init(x: 0, y: 0, z: 0)
    public var material: Material = .init()
    public var distance: LightDistance = .distance50
    public var innerCutoff: Float = 15.0
    public var outerCutoff: Float = 25.0

    var debugObject: CoreObject?

    init() {
        self.debugObject = nil
    }

    public static func pointLight(position: Position3d, color: Color = .shadeOfWhite(1), intensity: Float = 1.0) -> Light {
        let light = Light()
        light.position = position
        light.type = .pointLight
        light.color = color
        light.intensity = intensity
        light.material.diffuse = [0.8, 0.8, 0.8]
        return light
    }

    public static func directionalLight(direction: Direction3d, color: Color = .shadeOfWhite(1), intensity: Float = 1.0) -> Light {
        let light = Light()
        light.direction = direction
        light.type = .directionalLight
        light.color = color
        light.intensity = intensity
        light.material.diffuse = [0.8, 0.8, 0.8]
        return light
    }
    
    public static func spotlight(position: Position3d, direction: Direction3d, color: Color = .shadeOfWhite(1), intensity: Float = 1.0) -> Light {
        let light = Light()
        light.direction = direction
        light.position = position
        light.type = .spotlight
        light.color = color
        light.intensity = intensity
        light.material.diffuse = [0.8, 0.8, 0.8]
        return light

    }

    func toMetalLight() -> MetalLight {
        let attenuationProperties = distance.getProperties()
        return MetalLight(type: type.rawValue, color: color.toSimd(), position: position.toSimd4(), direction: direction.toSimd4(), intensity: intensity, specular: material.specular.toSimd(), diffuse: material.diffuse.toSimd(), constant: attenuationProperties.constant, linear: attenuationProperties.linear, quadratic: attenuationProperties.quadratic, innerCutoff: cos(radians(fromDegrees: innerCutoff)), outerCutoff: cos(radians(fromDegrees: outerCutoff)))
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
