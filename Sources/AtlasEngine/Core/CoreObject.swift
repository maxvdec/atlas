//
//  CoreObject.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import MetalKit
import Metal
import Foundation
import simd

struct CoreVertex {
    var position: Position3d
    
    public func toMetalVertex() -> MetalVertex {
        return MetalVertex(position: self.position.toSimd())
    }
}

struct MetalVertex {
    var position: SIMD3<Float>
}

// TODO: Remove the 'public' for release
public class CoreObject: Identifiable {
    public let id: UUID
    var vertices: [CoreVertex]
    var dispatcher: RenderDispatch?
    var pipelineState: MTLRenderPipelineState?
    
    var vertexBuffer: MTLBuffer?
    
    init() {
        self.id = UUID()
        vertices = []
    }
    
    init(vertices: [CoreVertex]) {
        self.id = UUID()
        self.vertices = []
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
        device.makeBuffer(bytes: vertexData, length: MemoryLayout<MetalVertex>.stride * vertexData.count, options: [])
    }
}
