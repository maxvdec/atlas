//
//  Material.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

struct MetalMaterial {
    var specularStrength: Float = 0.5
}

public struct Material {
    public var specularStrength: Float = 0.5

    func toMetalMaterial() -> MetalMaterial {
        return MetalMaterial(specularStrength: specularStrength)
    }
}
