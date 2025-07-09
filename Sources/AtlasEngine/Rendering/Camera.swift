//
//  Camera.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import Metal
import MetalKit
import simd

public class Camera: Interactive {
    public var position: Position3d {
        didSet {
            updateViewMatrix()
        }
    }

    private var front: SIMD3<Float> = .init(0.0, 0.0, -1.0)
    private var up: SIMD3<Float> = .init(0.0, 1.0, 0.0)
    private var right: SIMD3<Float> = .init(1.0, 0.0, 0.0)
    private let worldUp: SIMD3<Float> = .init(0.0, 1.0, 0.0)

    private var yaw: Float = -90.0 {
        didSet {
            updateVectors()
        }
    }

    private var pitch: Float = 0.0 {
        didSet {
            updateVectors()
        }
    }

    public var fov: Float = 60.0 {
        didSet {
            updateProjectionMatrix()
        }
    }

    public var aspectRatio: Float = 16.0 / 9.0 {
        didSet {
            updateProjectionMatrix()
        }
    }

    public var farPlane: Float = 100.0 {
        didSet {
            updateProjectionMatrix()
        }
    }

    public var nearPlane: Float = 0.1 {
        didSet {
            updateProjectionMatrix()
        }
    }

    public var movementSpeed: Float = 2.5
    public var mouseSensitivity: Float = 0.1

    private var cachedViewMatrix: simd_float4x4 = matrix_identity_float4x4
    private var cachedProjectionMatrix: simd_float4x4 = matrix_identity_float4x4

    public func eachFrame(_ deltaTime: Double) {
        let dt = Float(deltaTime)

        let newAspectRatio = RenderDispatcher.shared.aspectRatio
        if abs(newAspectRatio - aspectRatio) > 0.001 {
            aspectRatio = newAspectRatio
        }

        handleMovementInput(deltaTime: dt)

        RenderDispatcher.shared.viewMatrix = viewMatrix
        RenderDispatcher.shared.projectionMatrix = projectionMatrix
    }

    public init(position: Position3d) {
        self.position = position
        updateVectors()
        updateViewMatrix()
        updateProjectionMatrix()
        RenderDispatcher.shared.registerFrameObject(self)
    }

    private func handleMovementInput(deltaTime: Float) {
        let velocity = movementSpeed * deltaTime

        if Key.isPressed(.w) {
            moveForward(velocity)
        }
        if Key.isPressed(.s) {
            moveForward(-velocity)
        }
        if Key.isPressed(.d) {
            moveRight(velocity)
        }
        if Key.isPressed(.a) {
            moveRight(-velocity)
        }
        if Key.isPressed(.space) {
            moveUp(velocity)
        }
        if Key.isPressed(.rightShift) {
            moveUp(-velocity)
        }
    }

    private func updateVectors() {
        let yawRad = radians(fromDegrees: yaw)
        let pitchRad = radians(fromDegrees: pitch)

        var direction = SIMD3<Float>()
        direction.x = cos(yawRad) * cos(pitchRad)
        direction.y = sin(pitchRad)
        direction.z = sin(yawRad) * cos(pitchRad)

        front = normalize(direction)
        right = normalize(cross(front, worldUp))
        up = normalize(cross(right, front))

        updateViewMatrix()
    }

    private func updateViewMatrix() {
        cachedViewMatrix = lookAt(eye: position.toSimd(), center: position.toSimd() + front, up: up)
    }

    private func updateProjectionMatrix() {
        let fovRadians = radians(fromDegrees: fov)
        cachedProjectionMatrix = perspectiveFovRH(fovRadians, aspectRatio, nearPlane, farPlane)
    }

    public var viewMatrix: simd_float4x4 {
        return cachedViewMatrix
    }

    public var projectionMatrix: simd_float4x4 {
        return cachedProjectionMatrix
    }

    public func getLookAtPoint(at distance: Float = 1.0) -> Position3d {
        let lookPoint = position.toSimd() + front * distance
        return Position3d(x: lookPoint.x, y: lookPoint.y, z: lookPoint.z)
    }

    public func moveForward(_ delta: Float) {
        let movement = front * delta
        position = Position3d(
            x: position.x + movement.x,
            y: position.y + movement.y,
            z: position.z + movement.z
        )
    }

    public func moveRight(_ delta: Float) {
        let movement = right * delta
        position = Position3d(
            x: position.x + movement.x,
            y: position.y + movement.y,
            z: position.z + movement.z
        )
    }

    public func moveUp(_ delta: Float) {
        let movement = worldUp * delta
        position = Position3d(
            x: position.x + movement.x,
            y: position.y + movement.y,
            z: position.z + movement.z
        )
    }

    public func rotate(yaw deltaYaw: Float, pitch deltaPitch: Float) {
        yaw += deltaYaw * mouseSensitivity
        pitch += deltaPitch * mouseSensitivity

        pitch = clamp(pitch, -89.0, 89.0)
    }

    public func onMouseMove(_ delta: Size2d) {
        processMouseMovement(xOffset: delta.width, yOffset: -delta.height)
    }

    public func onMouseScroll(_ yOffset: Float) {
        processMouseScroll(yOffset: yOffset)
    }

    public func processMouseMovement(xOffset: Float, yOffset: Float) {
        rotate(yaw: xOffset, pitch: yOffset)
    }

    public func processMouseScroll(yOffset: Float) {
        let scrollSensitivity: Float = 1.0
        fov -= yOffset * scrollSensitivity
        fov = clamp(fov, 1.0, 90.0)
    }

    public var frontVector: SIMD3<Float> { return front }
    public var upVector: SIMD3<Float> { return up }
    public var rightVector: SIMD3<Float> { return right }
    public var yawAngle: Float { return yaw }
    public var pitchAngle: Float { return pitch }
}

func radians(fromDegrees degrees: Float) -> Float {
    return degrees * .pi / 180.0
}

func clamp<T: Comparable>(_ value: T, _ minVal: T, _ maxVal: T) -> T {
    return max(minVal, min(value, maxVal))
}

func lookAt(eye: SIMD3<Float>, center: SIMD3<Float>, up: SIMD3<Float>) -> simd_float4x4 {
    let f = normalize(center - eye)
    let s = normalize(cross(f, up))
    let u = cross(s, f)

    var result = matrix_identity_float4x4
    result.columns.0 = SIMD4<Float>(s.x, u.x, -f.x, 0)
    result.columns.1 = SIMD4<Float>(s.y, u.y, -f.y, 0)
    result.columns.2 = SIMD4<Float>(s.z, u.z, -f.z, 0)
    result.columns.3 = SIMD4<Float>(-dot(s, eye), -dot(u, eye), dot(f, eye), 1)
    return result
}

func perspectiveFovRH(_ fovY: Float, _ aspect: Float, _ nearZ: Float, _ farZ: Float) -> simd_float4x4 {
    let yScale = 1 / tan(fovY * 0.5)
    let xScale = yScale / aspect
    let zRange = farZ - nearZ
    let zScale = -(farZ + nearZ) / zRange
    let wzScale = -2 * farZ * nearZ / zRange

    return simd_float4x4(
        SIMD4<Float>(xScale, 0, 0, 0),
        SIMD4<Float>(0, yScale, 0, 0),
        SIMD4<Float>(0, 0, zScale, -1),
        SIMD4<Float>(0, 0, wzScale, 0)
    )
}

func makeDefaultViewMatrix() -> matrix_float4x4 {
    let eye = SIMD3<Float>(0, 0, 0)
    let center = SIMD3<Float>(0, 0, -1)
    let up = SIMD3<Float>(0, 1, 0)
    return lookAt(eye: eye, center: center, up: up)
}

func makeDefaultProjectionMatrix(aspect: Float) -> matrix_float4x4 {
    let fov: Float = 60.0 * .pi / 180.0
    let near: Float = 0.1
    let far: Float = 100.0
    return perspectiveFovRH(fov, aspect, near, far)
}

func orthographicProjection(left: Float, right: Float, bottom: Float, top: Float, near: Float, far: Float) -> simd_float4x4 {
    let ral = right + left
    let rsl = right - left
    let tab = top + bottom
    let tsb = top - bottom
    let fan = far + near
    let fsn = far - near

    return simd_float4x4(
        SIMD4(2 / rsl, 0, 0, 0),
        SIMD4(0, 2 / tsb, 0, 0),
        SIMD4(0, 0, -2 / fsn, 0),
        SIMD4(-ral / rsl, -tab / tsb, -fan / fsn, 1)
    )
}
