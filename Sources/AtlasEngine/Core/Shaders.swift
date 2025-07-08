//
//  Shaders.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import Metal
import MetalKit

protocol CoreShader {
    func makePipeline(device: MTLDevice) -> MTLRenderPipelineState
    func makeUniforms(coreObject: CoreObject) -> MTLBuffer
}

struct BasicShaderUniforms {
    var textureCount: Int32
    var model: simd_float4x4
}

/// Shader that renders the object with a solid color and no lighting nor transformation
class BasicShader: CoreShader {
    public func makePipeline(device: any MTLDevice) -> any MTLRenderPipelineState {
        if RenderDispatcher.shared.library == nil {
            RenderDispatcher.shared.library = try! device.makeLibrary(source: allMetalShaders, options: nil)
            if RenderDispatcher.shared.library == nil {
                fatalError("Could not create library")
            }
        }

        let library = RenderDispatcher.shared.library!
        let vertex = library.makeFunction(name: "basic_vertex")!
        let fragment = library.makeFunction(name: "basic_fragment")!

        // According to the shader, we need to create a Vertex Descriptor
        let vertexDescriptor = makeVertexDescriptor()

        // We use a Pipeline Descriptor to create the Rendering Pipeline
        let pipeline = MTLRenderPipelineDescriptor()
        pipeline.vertexFunction = vertex
        pipeline.fragmentFunction = fragment
        pipeline.colorAttachments[0].pixelFormat = .bgra8Unorm

        pipeline.vertexDescriptor = vertexDescriptor

        do {
            return try device.makeRenderPipelineState(descriptor: pipeline)
        } catch {
            fatalError("Failed to create pipeline: \(error)")
        }
    }

    public func makeUniforms(coreObject: CoreObject) -> any MTLBuffer {
        var uniforms = BasicShaderUniforms(textureCount: Int32(coreObject.textures.count), model: coreObject.model)
        let uniformBuffer = RenderDispatcher.shared.device.makeBuffer(length: MemoryLayout<BasicShaderUniforms>.stride, options: .storageModeShared)!
        let bufferPointer = uniformBuffer.contents()
        memcpy(bufferPointer, &uniforms, MemoryLayout<BasicShaderUniforms>.stride)

        return uniformBuffer
    }
}

func makeVertexDescriptor() -> MTLVertexDescriptor {
    let vertexDescriptor = MTLVertexDescriptor()
    vertexDescriptor.layouts[0].stride = MemoryLayout<MetalVertex>.stride
    vertexDescriptor.layouts[0].stepRate = 1
    vertexDescriptor.layouts[0].stepFunction = .perVertex

    var offset = 0

    // Position (x, y, z)
    vertexDescriptor.attributes[0].format = .float3
    vertexDescriptor.attributes[0].offset = offset
    vertexDescriptor.attributes[0].bufferIndex = 0

    offset += MemoryLayout<SIMD3<Float>>.stride

    // Color (r, g, b, a)
    vertexDescriptor.attributes[1].format = .float4
    vertexDescriptor.attributes[1].offset = offset
    vertexDescriptor.attributes[1].bufferIndex = 0

    offset += MemoryLayout<SIMD4<Float>>.stride

    // Texture coordinates
    vertexDescriptor.attributes[2].format = .float2
    vertexDescriptor.attributes[2].offset = offset
    vertexDescriptor.attributes[2].bufferIndex = 0

    return vertexDescriptor
}
