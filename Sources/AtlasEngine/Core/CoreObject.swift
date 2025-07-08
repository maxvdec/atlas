//
//  CoreObject.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import Foundation
import Metal
import MetalKit
import simd

public struct CoreVertex {
    public var position: Position3d
    public var color: Color
    public var texCoord: Position2d
    public var normal: Magnitude3d
    
    public init(position: Position3d, color: Color, texCoordinates: Position2d = [0, 0], normal: Magnitude3d = [0, 0, 0]) {
        self.position = position
        self.color = color
        self.texCoord = texCoordinates
        self.normal = normal
    }
    
    func toMetalVertex() -> MetalVertex {
        return MetalVertex(position: position.toSimd(), color: color.toSimd(), texCoordinates: texCoord.toSimd(), normal: normal.toSimd())
    }
}

struct MetalVertex {
    var position: SIMD3<Float>
    var color: SIMD4<Float>
    var texCoordinates: SIMD2<Float>
    var normal: SIMD3<Float>
}

public typealias PrimitiveIndex = UInt16

// TODO: Remove the 'public' for release
public class CoreObject: Identifiable {
    public let id: UUID
    var sampler: TextureSampler = .init()
    var vertices: [CoreVertex]
    var indices: [PrimitiveIndex]?
    var dispatcher: RenderDispatch?
    var pipelineState: MTLRenderPipelineState?
    var samplerState: MTLSamplerState?
    var useIndexedDrawing: Bool = false
    var isRendering: Bool = true
    
    public var material: Material = .init()
    
    var vertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var uniformsBuffer: MTLBuffer?
    
    var position: Position3d = [0, 0, 0]
    var rotation: Magnitude3d = [0, 0, 0]
    var scale: Size3d = [1, 1, 1]
    
    var model: float4x4 = .init()
    
    var shader: CoreShader = PhongShader()
    
    var textures: [Texture] = []
    
    public init() {
        self.id = UUID()
        self.vertices = []
    }
    
    public init(vertices: [CoreVertex]) {
        self.id = UUID()
        self.vertices = vertices
    }
    
    func setVertices(_ vertices: [CoreVertex]) {
        self.vertices = vertices
    }
    
    public func hide() {
        isRendering = false
    }
    
    public func show() {
        isRendering = true
    }
    
    public func setIndices(_ indices: [PrimitiveIndex]) {
        self.indices = indices
        useIndexedDrawing = true
    }
    
    public func colorVertices(with color: Color) {
        for i in vertices.indices {
            vertices[i].color = color
        }
    }
    
    public func scale(by magnitude: Size3d) {
        scale += magnitude
    }
    
    public func move(by offset: Position3d) {
        position += offset
    }
    
    public func rotate(by magnitude: Magnitude3d) {
        rotation += magnitude
    }
    
    public func attachTexture(_ texture: Texture) {
        textures.append(texture)
    }
    
    public func attachTextureCoordinates(_ coordinates: [Position2d]) {
        if coordinates.count != vertices.count {
            fatalError("Cannot load coordinates. The length was \(coordinates.count), but the number of vertices is \(vertices.count).")
        }
        for i in vertices.indices {
            vertices[i].texCoord = coordinates[i]
        }
    }
    
    func makeModelMatrix() -> simd_float4x4 {
        let translation_matrix = simd_float4x4(translation: position.toSimd())
        let rotation_matrix = simd_float4x4(rotation: rotation.toSimd())
        let scale_matrix = simd_float4x4(scaling: scale.toSimd())
        
        return translation_matrix * rotation_matrix * scale_matrix
    }
    
    public func initialize() {
        // First, we build the vertex buffer
        var vertexData: [MetalVertex] = []
        for vertex in vertices {
            vertexData.append(vertex.toMetalVertex())
        }
        let device = RenderDispatcher.shared.device!
        vertexBuffer = device.makeBuffer(bytes: vertexData, length: MemoryLayout<MetalVertex>.stride * vertexData.count, options: [])
        
        if useIndexedDrawing {
            indexBuffer = device.makeBuffer(bytes: indices!, length: MemoryLayout<PrimitiveIndex>.stride * indices!.count, options: [])
        }
        
        // Then the pipeline state
        pipelineState = shader.makePipeline(device: device)
        
        // We then make the uniform buffer
        uniformsBuffer = shader.makeUniforms(coreObject: self)
        
        // We make the sampler state
        samplerState = sampler.makeMetalSampler(device: device)
        
        if shader.type == .phongShader {
            RenderDispatcher.shared.currentScene.ensureLightBuffer()
        }
        
        // We set the rendering logic
        dispatcher = { object, encoder in
            encoder.setRenderPipelineState(object.pipelineState!)
            encoder.setVertexBuffer(object.vertexBuffer!, offset: 0, index: 0)
            
            object.model = object.makeModelMatrix()
            
            object.updateUniforms()
            
            encoder.setVertexBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentSamplerState(object.samplerState!, index: 0)
            
            if object.shader.type == .phongShader {
                let scene = RenderDispatcher.shared.currentScene
                if let lightBuffer = scene.lightBuffer {
                    encoder.setFragmentBuffer(lightBuffer, offset: 0, index: 2)
                } else {
                    print("Warning: Light buffer is nil for Phong shader")
                }
            }
            
            for (i, texture) in object.textures.enumerated() {
                encoder.setFragmentTexture(texture.mtlTexture, index: i)
            }
            
            if object.useIndexedDrawing {
                encoder.drawIndexedPrimitives(type: .triangle, indexCount: object.indices!.count, indexType: .uint16, indexBuffer: object.indexBuffer!, indexBufferOffset: 0)
            } else {
                encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: object.vertices.count)
            }
        }
        
        // And we register the object
        RenderDispatcher.shared.registerObject(coreObject: self)
    }
    
    private func updateUniforms() {
        guard let uniformsBuffer = uniformsBuffer else { return }
        
        let newUniformBuffer = shader.makeUniforms(coreObject: self)
        
        let bufferSize = shader.uniformsSize()
        memcpy(uniformsBuffer.contents(), newUniformBuffer.contents(), bufferSize)
    }
}

// FIXED: Improved cube generation with proper normals
public func generateCubeObject(size: Size3d) -> CoreObject {
    let w = size.width / 2
    let h = size.height / 2
    let d = size.depth / 2

    let positions: [Position3d] = [
        [-w, -h, -d], // 0
        [w, -h, -d], // 1
        [w, h, -d], // 2
        [-w, h, -d], // 3
        [-w, -h, d], // 4
        [w, -h, d], // 5
        [w, h, d], // 6
        [-w, h, d], // 7
    ]
    
    let uvs: [Position2d] = [
        [0, 0], [1, 0], [1, 1], [0, 1],
    ]

    let color: Color = [1, 1, 1] // default white color

    // FIXED: Proper face normals for cube
    let normals: [Magnitude3d] = [
        [0, 0, -1], // back face
        [0, 0, 1], // front face
        [-1, 0, 0], // left face
        [1, 0, 0], // right face
        [0, 1, 0], // top face
        [0, -1, 0], // bottom face
    ]

    let vertices: [CoreVertex] = [
        // Back face (facing -Z)
        CoreVertex(position: positions[0], color: color, texCoordinates: uvs[0], normal: normals[0]),
        CoreVertex(position: positions[1], color: color, texCoordinates: uvs[1], normal: normals[0]),
        CoreVertex(position: positions[2], color: color, texCoordinates: uvs[2], normal: normals[0]),
        CoreVertex(position: positions[3], color: color, texCoordinates: uvs[3], normal: normals[0]),

        // Front face (facing +Z)
        CoreVertex(position: positions[4], color: color, texCoordinates: uvs[0], normal: normals[1]),
        CoreVertex(position: positions[5], color: color, texCoordinates: uvs[1], normal: normals[1]),
        CoreVertex(position: positions[6], color: color, texCoordinates: uvs[2], normal: normals[1]),
        CoreVertex(position: positions[7], color: color, texCoordinates: uvs[3], normal: normals[1]),

        // Left face (facing -X)
        CoreVertex(position: positions[0], color: color, texCoordinates: uvs[0], normal: normals[2]),
        CoreVertex(position: positions[4], color: color, texCoordinates: uvs[1], normal: normals[2]),
        CoreVertex(position: positions[7], color: color, texCoordinates: uvs[2], normal: normals[2]),
        CoreVertex(position: positions[3], color: color, texCoordinates: uvs[3], normal: normals[2]),

        // Right face (facing +X)
        CoreVertex(position: positions[1], color: color, texCoordinates: uvs[0], normal: normals[3]),
        CoreVertex(position: positions[5], color: color, texCoordinates: uvs[1], normal: normals[3]),
        CoreVertex(position: positions[6], color: color, texCoordinates: uvs[2], normal: normals[3]),
        CoreVertex(position: positions[2], color: color, texCoordinates: uvs[3], normal: normals[3]),

        // Top face (facing +Y)
        CoreVertex(position: positions[3], color: color, texCoordinates: uvs[0], normal: normals[4]),
        CoreVertex(position: positions[2], color: color, texCoordinates: uvs[1], normal: normals[4]),
        CoreVertex(position: positions[6], color: color, texCoordinates: uvs[2], normal: normals[4]),
        CoreVertex(position: positions[7], color: color, texCoordinates: uvs[3], normal: normals[4]),

        // Bottom face (facing -Y)
        CoreVertex(position: positions[0], color: color, texCoordinates: uvs[0], normal: normals[5]),
        CoreVertex(position: positions[1], color: color, texCoordinates: uvs[1], normal: normals[5]),
        CoreVertex(position: positions[5], color: color, texCoordinates: uvs[2], normal: normals[5]),
        CoreVertex(position: positions[4], color: color, texCoordinates: uvs[3], normal: normals[5]),
    ]

    // Index list for triangles (two per face)
    let indices: [PrimitiveIndex] = [
        0, 1, 2, 2, 3, 0, // back
        4, 5, 6, 6, 7, 4, // front
        8, 9, 10, 10, 11, 8, // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20, // bottom
    ]

    let cube = CoreObject(vertices: vertices)
    cube.setIndices(indices)
    return cube
}

extension simd_float4x4 {
    init(translation: SIMD3<Float>) {
        self = matrix_identity_float4x4
        columns.3 = SIMD4<Float>(translation, 1)
    }
    
    init(scaling: SIMD3<Float>) {
        self = matrix_identity_float4x4
        columns.0.x = scaling.x
        columns.1.y = scaling.y
        columns.2.z = scaling.z
    }

    init(rotation: SIMD3<Float>) {
        let rotationX = simd_float4x4(rotationX: rotation.x)
        let rotationY = simd_float4x4(rotationY: rotation.y)
        let rotationZ = simd_float4x4(rotationZ: rotation.z)
        self = rotationZ * rotationY * rotationX
    }
    
    init(rotationX angle: Float) {
        self = matrix_identity_float4x4
        columns.1.y = cos(angle)
        columns.1.z = sin(angle)
        columns.2.y = -sin(angle)
        columns.2.z = cos(angle)
    }

    init(rotationY angle: Float) {
        self = matrix_identity_float4x4
        columns.0.x = cos(angle)
        columns.0.z = -sin(angle)
        columns.2.x = sin(angle)
        columns.2.z = cos(angle)
    }

    init(rotationZ angle: Float) {
        self = matrix_identity_float4x4
        columns.0.x = cos(angle)
        columns.0.y = sin(angle)
        columns.1.x = -sin(angle)
        columns.1.y = cos(angle)
    }
}
