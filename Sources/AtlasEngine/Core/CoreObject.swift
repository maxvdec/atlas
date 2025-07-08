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
    
    public init(position: Position3d, color: Color, texCoordinates: Position2d = [0, 0]) {
        self.position = position
        self.color = color
        self.texCoord = texCoordinates
    }
    
    func toMetalVertex() -> MetalVertex {
        return MetalVertex(position: position.toSimd(), color: color.toSimd(), texCoordinates: texCoord.toSimd())
    }
}

struct MetalVertex {
    var position: SIMD3<Float>
    var color: SIMD4<Float>
    var texCoordinates: SIMD2<Float>
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
    
    var vertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var uniformsBuffer: MTLBuffer?
    
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
    
    public func setIndices(_ indices: [PrimitiveIndex]) {
        self.indices = indices
        useIndexedDrawing = true
    }
    
    public func colorVertices(with color: Color) {
        for i in vertices.indices {
            vertices[i].color = color
        }
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
        let shader = BasicShader()
        pipelineState = shader.makePipeline(device: device)
        
        // We then make the uniform buffer
        uniformsBuffer = shader.makeUniforms(coreObject: self)
        
        // We make the sampler state
        samplerState = sampler.makeMetalSampler(device: device)
        
        // We set the rendering logic
        dispatcher = { object, encoder in
            encoder.setRenderPipelineState(object.pipelineState!)
            encoder.setVertexBuffer(object.vertexBuffer!, offset: 0, index: 0)
            encoder.setVertexBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentBuffer(object.uniformsBuffer!, offset: 0, index: 1)
            encoder.setFragmentSamplerState(object.samplerState!, index: 0)
            
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
}

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

    let vertices: [CoreVertex] = [
        CoreVertex(position: positions[0], color: color, texCoordinates: uvs[0]),
        CoreVertex(position: positions[1], color: color, texCoordinates: uvs[1]),
        CoreVertex(position: positions[2], color: color, texCoordinates: uvs[2]),
        CoreVertex(position: positions[3], color: color, texCoordinates: uvs[3]),
        CoreVertex(position: positions[4], color: color, texCoordinates: uvs[0]),
        CoreVertex(position: positions[5], color: color, texCoordinates: uvs[1]),
        CoreVertex(position: positions[6], color: color, texCoordinates: uvs[2]),
        CoreVertex(position: positions[7], color: color, texCoordinates: uvs[3]),
    ]

    // Index list for triangles (two per face)
    let indices: [PrimitiveIndex] = [
        // back face
        0, 1, 2, 2, 3, 0,
        // front face
        4, 5, 6, 6, 7, 4,
        // left face
        0, 4, 7, 7, 3, 0,
        // right face
        1, 5, 6, 6, 2, 1,
        // top face
        3, 2, 6, 6, 7, 3,
        // bottom face
        0, 1, 5, 5, 4, 0,
    ]

    let cube = CoreObject(vertices: vertices)
    cube.setIndices(indices)
    return cube
}
