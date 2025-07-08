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
        self.x = elements.count > 0 ? elements[0] : 0
        self.y = elements.count > 1 ? elements[1] : 0
        self.z = elements.count > 2 ? elements[2] : 0
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
    
    static func +=(lhs: inout Position3d, rhs: Position3d) {
        lhs = rhs + lhs
    }
}

public typealias Magnitude3d = Position3d

public struct Size3d: Equatable, ExpressibleByArrayLiteral {
    public var width: Float
    public var height: Float
    public var depth: Float

    public init(width: Float, height: Float, depth: Float) {
        self.width = width
        self.height = height
        self.depth = depth
    }

    public init(arrayLiteral elements: Float...) {
        self.width = elements.count > 0 ? elements[0] : 0
        self.height = elements.count > 1 ? elements[1] : 0
        self.depth = elements.count > 2 ? elements[2] : 0
    }

    public func toSimd() -> SIMD3<Float> {
        SIMD3<Float>(width, height, depth)
    }
}

public extension Size3d {
    static func ==(lhs: Size3d, rhs: Size3d) -> Bool {
        lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth
    }

    static func +(lhs: Size3d, rhs: Size3d) -> Size3d {
        Size3d(width: lhs.width + rhs.width, height: lhs.height + rhs.height, depth: lhs.depth + rhs.depth)
    }

    static func -(lhs: Size3d, rhs: Size3d) -> Size3d {
        Size3d(width: lhs.width - rhs.width, height: lhs.height - rhs.height, depth: lhs.depth - rhs.depth)
    }

    static func *(lhs: Size3d, rhs: Float) -> Size3d {
        Size3d(width: lhs.width * rhs, height: lhs.height * rhs, depth: lhs.depth * rhs)
    }

    static func /(lhs: Size3d, rhs: Float) -> Size3d {
        Size3d(width: lhs.width / rhs, height: lhs.height / rhs, depth: lhs.depth / rhs)
    }
    
    static func +=(lhs: inout Size3d, rhs: Size3d) {
        lhs = lhs + rhs
    }
}

public struct Position2d: Equatable, ExpressibleByArrayLiteral {
    public var x: Float
    public var y: Float

    public init(x: Float, y: Float) {
        self.x = x
        self.y = y
    }

    public init(arrayLiteral elements: Float...) {
        self.x = elements.count > 0 ? elements[0] : 0
        self.y = elements.count > 1 ? elements[1] : 0
    }

    public func toSimd() -> SIMD2<Float> {
        return SIMD2<Float>(x, y)
    }
}

public extension Position2d {
    static func ==(lhs: Position2d, rhs: Position2d) -> Bool {
        return lhs.x == rhs.x && lhs.y == rhs.y
    }

    static func +(lhs: Position2d, rhs: Position2d) -> Position2d {
        return Position2d(x: lhs.x + rhs.x, y: lhs.y + rhs.y)
    }

    static func -(lhs: Position2d, rhs: Position2d) -> Position2d {
        return Position2d(x: lhs.x - rhs.x, y: lhs.y - rhs.y)
    }

    static func *(lhs: Position2d, rhs: Float) -> Position2d {
        return Position2d(x: lhs.x * rhs, y: lhs.y * rhs)
    }

    static func /(lhs: Position2d, rhs: Float) -> Position2d {
        return Position2d(x: lhs.x / rhs, y: lhs.y / rhs)
    }
}

public struct Color: ExpressibleByArrayLiteral {
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

    public init(arrayLiteral elements: Float...) {
        self.r = elements.count > 0 ? elements[0] : 0
        self.g = elements.count > 1 ? elements[1] : 0
        self.b = elements.count > 2 ? elements[2] : 0
        self.a = elements.count > 3 ? elements[3] : 1
    }

    static func shadeOfWhite(_ val: Float) -> Color {
        return Color(r: val, g: val, b: val)
    }

    func toSimd() -> SIMD4<Float> {
        return SIMD4<Float>(r, g, b, a)
    }
}
