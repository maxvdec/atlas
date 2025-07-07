//
//  Units.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import simd

struct Position3d: Equatable {
    var x: Float
    var y: Float
    var z: Float
}

extension Position3d {
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
