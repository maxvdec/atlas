//
//  Units.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import simd

public struct Position3d: Equatable, ExpressibleByArrayLiteral {
    public var x: Float
    public var y: Float
    public var z: Float
    
    public init(x: Float, y: Float, z: Float) {
        self.x = x
        self.y = y
        self.z = z
    }
    
    public init(arrayLiteral elements: Float...) {
        precondition(elements.count == 3, "Position3d requires exactly three elements: x, y, z.")
        self.x = elements[0]
        self.y = elements[1]
        self.z = elements[2]
    }
    
    public func toSimd() -> SIMD3<Float> {
        return SIMD3<Float>(x, y, z)
    }
}

public extension Position3d {
    static func ==(lhs: Position3d, rhs: Position3d) -> Bool {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z
    }
    
    static func +(lhs: Position3d, rhs: Position3d) -> Position3d {
        return Position3d(x: lhs.x + rhs.x, y: lhs.y + rhs.y, z: lhs.z + rhs.z)
    }
    
    static func -(lhs: Position3d, rhs: Position3d) -> Position3d {
        return Position3d(x: lhs.x - rhs.x, y: lhs.y - rhs.y, z: lhs.z - rhs.z)
    }
    
    static func *(lhs: Position3d, rhs: Float) -> Position3d {
        return Position3d(x: lhs.x * rhs, y: lhs.y * rhs, z: lhs.z * rhs)
    }
    
    static func /(lhs: Position3d, rhs: Float) -> Position3d {
        return Position3d(x: lhs.x / rhs, y: lhs.y / rhs, z: lhs.z / rhs)
    }
}

public struct Color {
    public var r: Float
    public var g: Float
    public var b: Float
    public var a: Float = 1.0
    
    public init(r: Float, g: Float, b: Float, a: Float = 1.0) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }
    
    static func shadeOfWhite(_ val: Float) -> Color {
        return Color(r: val, g: val, b: val)
    }
    
    func toSimd() -> SIMD4<Float> {
        return SIMD4<Float>(r, g, b, a)
    }
}
