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
        return MetalVertex(position: position.toSimd4(), color: color.toSimd(), texCoordinates: texCoord.toSimd(), _texPadding: .init(0, 0), normal: normal.toSimd4())
    }
    
    func toDepthVertex() -> DepthShaderVertex {
        return DepthShaderVertex(position: position.toSimd4())
    }
}

struct MetalVertex {
    var position: SIMD4<Float>
    var color: SIMD4<Float>
    var texCoordinates: SIMD2<Float>
    var _texPadding: SIMD2<Float> = .init(0, 0)
    var normal: SIMD4<Float>
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
    var depthVertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var uniformsBuffer: MTLBuffer?
    
    var position: Position3d = [0, 0, 0] {
        didSet {
            model = makeModelMatrix()
            RenderDispatcher.shared.remakeDepthMaps = true
        }
    }

    var rotation: Magnitude3d = [0, 0, 0] {
        didSet {
            model = makeModelMatrix()
            RenderDispatcher.shared.remakeDepthMaps = true
        }
    }

    var scale: Size3d = [1, 1, 1] {
        didSet {
            model = makeModelMatrix()
        }
    }
    
    var model: float4x4 = .init()
    
    var shader: CoreShader = ShadowShader()
    
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
    
    func initializeCore() {
        // First, we build the vertex buffer
        var vertexData: [MetalVertex] = []
        var depthVertexData: [DepthShaderVertex] = []
        for vertex in vertices {
            vertexData.append(vertex.toMetalVertex())
            depthVertexData.append(vertex.toDepthVertex())
        }
        let device = RenderDispatcher.shared.device!
        vertexBuffer = device.makeBuffer(bytes: vertexData, length: MemoryLayout<MetalVertex>.stride * vertexData.count, options: [])
        depthVertexBuffer = device.makeBuffer(bytes: depthVertexData, length: MemoryLayout<DepthShaderVertex>.stride * depthVertexData.count, options: [])
        
        if useIndexedDrawing {
            indexBuffer = device.makeBuffer(bytes: indices!, length: MemoryLayout<PrimitiveIndex>.stride * indices!.count, options: [])
        }
        
        // Then the pipeline state
        pipelineState = shader.makePipeline(device: device)
        
        // We make the sampler state
        samplerState = sampler.makeMetalSampler(device: device)
        
        if shader.type != .basicShader && shader.type != .fullscreenShader {
            RenderDispatcher.shared.currentScene.ensureLightBuffer()
        }
        
        model = makeModelMatrix()
    }
    
    func initializeDispatcher() {
        // We set the rendering logic
        dispatcher = { object, encoder in
            encoder.setRenderPipelineState(object.pipelineState!)
            encoder.setVertexBuffer(object.vertexBuffer!, offset: 0, index: 0)
            
            if RenderDispatcher.shared.remakeUniforms {
                object.uniformsBuffer = object.shader.makeUniforms(coreObject: object)
            }
             
            encoder.setVertexBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentSamplerState(object.samplerState!, index: 0)
             
            if object.shader.type != .basicShader || object.shader.type != .fullscreenShader {
                let scene = RenderDispatcher.shared.currentScene
                if let lightBuffer = scene.lightBuffer {
                    encoder.setFragmentBuffer(lightBuffer, offset: 0, index: 2)
                } else {
                    print("Warning: Light buffer is nil for Phong shader")
                }
            }
             
            for (i, texture) in object.textures.enumerated() {
                if texture.type == .color {
                    encoder.setFragmentTexture(texture.mtlTexture, index: i)
                } else if texture.type == .specular {
                    encoder.setFragmentTexture(texture.mtlTexture, index: i + 3) // We leave 3 slots for color textures
                }
            }
            
            if (RenderDispatcher.shared.currentScene.lights.filter { $0.castsShadows }).count > 0 {
                encoder.setFragmentTexture((RenderDispatcher.shared.currentScene.lights.filter { $0.castsShadows }).first?.shadowRenderer.depthTexture, index: 5)
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
    
    public func initialize() {
        initializeCore()
        initializeDispatcher()
    }
}

public func generateCubeObject(size: Size3d) -> CoreObject {
    let w = size.width / 2
    let h = size.height / 2
    let d = size.depth / 2
    
    let color: Color = [1, 1, 1]
    
    let vertices: [CoreVertex] = [
        // Front face (facing +Z) - normal: [0, 0, 1]
        CoreVertex(position: [-w, -h, d], color: color, texCoordinates: [0, 0], normal: [0, 0, 1]),
        CoreVertex(position: [w, -h, d], color: color, texCoordinates: [1, 0], normal: [0, 0, 1]),
        CoreVertex(position: [w, h, d], color: color, texCoordinates: [1, 1], normal: [0, 0, 1]),
        CoreVertex(position: [-w, h, d], color: color, texCoordinates: [0, 1], normal: [0, 0, 1]),

        // Back face (facing -Z) - normal: [0, 0, -1]
        CoreVertex(position: [w, -h, -d], color: color, texCoordinates: [0, 0], normal: [0, 0, -1]),
        CoreVertex(position: [-w, -h, -d], color: color, texCoordinates: [1, 0], normal: [0, 0, -1]),
        CoreVertex(position: [-w, h, -d], color: color, texCoordinates: [1, 1], normal: [0, 0, -1]),
        CoreVertex(position: [w, h, -d], color: color, texCoordinates: [0, 1], normal: [0, 0, -1]),

        // Left face (facing -X) - normal: [-1, 0, 0]
        CoreVertex(position: [-w, -h, -d], color: color, texCoordinates: [0, 0], normal: [-1, 0, 0]),
        CoreVertex(position: [-w, -h, d], color: color, texCoordinates: [1, 0], normal: [-1, 0, 0]),
        CoreVertex(position: [-w, h, d], color: color, texCoordinates: [1, 1], normal: [-1, 0, 0]),
        CoreVertex(position: [-w, h, -d], color: color, texCoordinates: [0, 1], normal: [-1, 0, 0]),

        // Right face (facing +X) - normal: [1, 0, 0]
        CoreVertex(position: [w, -h, d], color: color, texCoordinates: [0, 0], normal: [1, 0, 0]),
        CoreVertex(position: [w, -h, -d], color: color, texCoordinates: [1, 0], normal: [1, 0, 0]),
        CoreVertex(position: [w, h, -d], color: color, texCoordinates: [1, 1], normal: [1, 0, 0]),
        CoreVertex(position: [w, h, d], color: color, texCoordinates: [0, 1], normal: [1, 0, 0]),

        // Top face (facing +Y) - normal: [0, 1, 0]
        CoreVertex(position: [-w, h, d], color: color, texCoordinates: [0, 0], normal: [0, 1, 0]),
        CoreVertex(position: [w, h, d], color: color, texCoordinates: [1, 0], normal: [0, 1, 0]),
        CoreVertex(position: [w, h, -d], color: color, texCoordinates: [1, 1], normal: [0, 1, 0]),
        CoreVertex(position: [-w, h, -d], color: color, texCoordinates: [0, 1], normal: [0, 1, 0]),

        // Bottom face (facing -Y) - normal: [0, -1, 0]
        CoreVertex(position: [-w, -h, -d], color: color, texCoordinates: [0, 0], normal: [0, -1, 0]),
        CoreVertex(position: [w, -h, -d], color: color, texCoordinates: [1, 0], normal: [0, -1, 0]),
        CoreVertex(position: [w, -h, d], color: color, texCoordinates: [1, 1], normal: [0, -1, 0]),
        CoreVertex(position: [-w, -h, d], color: color, texCoordinates: [0, 1], normal: [0, -1, 0]),
    ]

    let indices: [PrimitiveIndex] = [
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Top face
        16, 17, 18, 18, 19, 16,
        // Bottom face
        20, 21, 22, 22, 23, 20,
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
