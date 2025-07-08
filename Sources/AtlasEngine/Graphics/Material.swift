//
//  Material.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

struct MetalMaterial {
    var ambient: SIMD3<Float> = .init()
    var diffuse: SIMD3<Float> = .init()
    var specular: SIMD3<Float> = .init()
    var shininess: Float = 32
}

public struct Material {
    public var ambient: Color = [0.1, 0.1, 0.1]
    public var diffuse: Color = [0.8, 0.8, 0.8]
    public var specular: Color = [0.5, 0.5, 0.5]
    public var shininess: Float = 32.0
    
    func toMetalMaterial() -> MetalMaterial {
        return MetalMaterial(
            ambient: ambient.toSimd3(),
            diffuse: diffuse.toSimd3(),
            specular: specular.toSimd3(),
            shininess: shininess
        )
    }
}
