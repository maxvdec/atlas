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
    
    public init(position: Position3d, color: Color) {
        self.position = position
        self.color = color
    }
    
    func toMetalVertex() -> MetalVertex {
        return MetalVertex(position: position.toSimd(), color: color.toSimd())
    }
}

struct MetalVertex {
    var position: SIMD3<Float>
    var color: SIMD4<Float>
}

// TODO: Remove the 'public' for release
public class CoreObject: Identifiable {
    public let id: UUID
    var vertices: [CoreVertex]
    var dispatcher: RenderDispatch?
    var pipelineState: MTLRenderPipelineState?
    
    var vertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    
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
    
    public func initialize() {
        // First, we build the vertex buffer
        var vertexData: [MetalVertex] = []
        for vertex in vertices {
            vertexData.append(vertex.toMetalVertex())
        }
        let device = RenderDispatcher.shared.device!
        vertexBuffer = device.makeBuffer(bytes: vertexData, length: MemoryLayout<MetalVertex>.stride * vertexData.count, options: [])
        
        // Then the pipeline state
        let shader = BasicShader()
        pipelineState = shader.makePipeline(device: device)
            
        // We set the rendering logic
        dispatcher = { object, encoder in
            encoder.setRenderPipelineState(object.pipelineState!)
            encoder.setVertexBuffer(object.vertexBuffer!, offset: 0, index: 0)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: object.vertices.count)
        }
        
        // And we register the object
        RenderDispatcher.shared.registerObject(coreObject: self)
    }
}
