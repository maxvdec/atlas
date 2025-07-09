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
    func uniformsSize() -> Int
    var type: CoreShaderType { get }
}

enum CoreShaderType {
    case basicShader
    case phongShader
}

struct BasicShaderUniforms {
    var textureCount: Int32
    var model: simd_float4x4 = .init()
    var view: simd_float4x4 = .init()
    var projection: simd_float4x4 = .init()
}

struct PhongShaderUniforms {
    var textureCount: Int32 = 0
    var specularMapCount: Int32 = 0
    var model: simd_float4x4 = .init()
    var view: simd_float4x4 = .init()
    var projection: simd_float4x4 = .init()
    var ambientColor: Color = []
    var lightCount: Int32 = 0
    var cameraPos: SIMD3<Float> = .init()
    var material: MetalMaterial = .init()
}

/// Shader that renders the object with a solid color and no lighting
class BasicShader: CoreShader {
    var type: CoreShaderType { .basicShader }
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
        pipeline.depthAttachmentPixelFormat = .depth32Float
        pipeline.rasterSampleCount = Atlas.preferences.sampleCount
        pipeline.colorAttachments[0].isBlendingEnabled = true

        pipeline.colorAttachments[0].rgbBlendOperation = .add
        pipeline.colorAttachments[0].alphaBlendOperation = .add
        pipeline.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipeline.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        pipeline.colorAttachments[0].sourceAlphaBlendFactor = .one
        pipeline.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        pipeline.vertexDescriptor = vertexDescriptor

        do {
            return try device.makeRenderPipelineState(descriptor: pipeline)
        } catch {
            fatalError("Failed to create pipeline: \(error)")
        }
    }

    public func makeUniforms(coreObject: CoreObject) -> any MTLBuffer {
        var uniforms = BasicShaderUniforms(textureCount: Int32(coreObject.textures.count), model: coreObject.model)
        uniforms.view = RenderDispatcher.shared.viewMatrix
        uniforms.projection = RenderDispatcher.shared.projectionMatrix
        let uniformBuffer = RenderDispatcher.shared.device.makeBuffer(length: MemoryLayout<BasicShaderUniforms>.stride, options: .storageModeShared)!
        let bufferPointer = uniformBuffer.contents()
        memcpy(bufferPointer, &uniforms, MemoryLayout<BasicShaderUniforms>.stride)

        return uniformBuffer
    }

    func uniformsSize() -> Int {
        return MemoryLayout<BasicShaderUniforms>.stride
    }
}

/// Shader that renders the object with a color or texture based on a simple lighting model
class PhongShader: CoreShader {
    var type: CoreShaderType { .phongShader }
    public func makePipeline(device: any MTLDevice) -> any MTLRenderPipelineState {
        if RenderDispatcher.shared.library == nil {
            RenderDispatcher.shared.library = try! device.makeLibrary(source: allMetalShaders, options: nil)
            if RenderDispatcher.shared.library == nil {
                fatalError("Could not create library")
            }
        }

        let library = RenderDispatcher.shared.library!
        let vertex = library.makeFunction(name: "phong_vertex")!
        let fragment = library.makeFunction(name: "phong_fragment")!

        // According to the shader, we need to create a Vertex Descriptor
        let vertexDescriptor = makeVertexDescriptor()

        // We use a Pipeline Descriptor to create the Rendering Pipeline
        let pipeline = MTLRenderPipelineDescriptor()
        pipeline.vertexFunction = vertex
        pipeline.fragmentFunction = fragment
        pipeline.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipeline.depthAttachmentPixelFormat = .depth32Float
        pipeline.rasterSampleCount = Atlas.preferences.sampleCount
        pipeline.colorAttachments[0].isBlendingEnabled = true

        pipeline.colorAttachments[0].rgbBlendOperation = .add
        pipeline.colorAttachments[0].alphaBlendOperation = .add
        pipeline.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipeline.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        pipeline.colorAttachments[0].sourceAlphaBlendFactor = .one
        pipeline.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha

        pipeline.vertexDescriptor = vertexDescriptor

        do {
            return try device.makeRenderPipelineState(descriptor: pipeline)
        } catch {
            fatalError("Failed to create pipeline: \(error)")
        }
    }

    public func makeUniforms(coreObject: CoreObject) -> any MTLBuffer {
        var uniforms = PhongShaderUniforms()
        let diffuseTextures = coreObject.textures.filter { $0.type == .color }
        let specularTextures = coreObject.textures.filter { $0.type == .specular }
        uniforms.specularMapCount = Int32(specularTextures.count)
        uniforms.textureCount = Int32(diffuseTextures.count)
        uniforms.model = coreObject.model
        uniforms.view = RenderDispatcher.shared.viewMatrix
        uniforms.projection = RenderDispatcher.shared.projectionMatrix
        uniforms.ambientColor = RenderDispatcher.shared.currentScene.ambientColor
        uniforms.lightCount = Int32(RenderDispatcher.shared.currentScene.lights.count)
        let camera = RenderDispatcher.shared.frameObjects.first(where: {
            $0 is Camera
        })
        if camera == nil {
            uniforms.cameraPos = ([0, 0, 0] as Position3d).toSimd()
        } else {
            uniforms.cameraPos = (camera as! Camera).position.toSimd()
        }
        uniforms.material = coreObject.material.toMetalMaterial()
        let uniformBuffer = RenderDispatcher.shared.device.makeBuffer(length: MemoryLayout<PhongShaderUniforms>.stride, options: .storageModeShared)!
        let bufferPointer = uniformBuffer.contents()
        memcpy(bufferPointer, &uniforms, MemoryLayout<PhongShaderUniforms>.stride)

        return uniformBuffer
    }

    func uniformsSize() -> Int {
        return MemoryLayout<PhongShaderUniforms>.stride
    }
}

func makeVertexDescriptor() -> MTLVertexDescriptor {
    let vertexDescriptor = MTLVertexDescriptor()
    vertexDescriptor.layouts[0].stride = MemoryLayout<MetalVertex>.stride
    vertexDescriptor.layouts[0].stepRate = 1
    vertexDescriptor.layouts[0].stepFunction = .perVertex

    var offset = 0

    // Position (x, y, z)
    vertexDescriptor.attributes[0].format = .float4
    vertexDescriptor.attributes[0].offset = offset
    vertexDescriptor.attributes[0].bufferIndex = 0

    offset += MemoryLayout<SIMD4<Float>>.stride

    // Color (r, g, b, a)
    vertexDescriptor.attributes[1].format = .float4
    vertexDescriptor.attributes[1].offset = offset
    vertexDescriptor.attributes[1].bufferIndex = 0

    offset += MemoryLayout<SIMD4<Float>>.stride

    // Texture coordinates
    vertexDescriptor.attributes[2].format = .float2
    vertexDescriptor.attributes[2].offset = offset
    vertexDescriptor.attributes[2].bufferIndex = 0

    offset += MemoryLayout<SIMD2<Float>>.stride * 2 // For padding

    // Normals
    vertexDescriptor.attributes[3].format = .float4
    vertexDescriptor.attributes[3].offset = offset
    vertexDescriptor.attributes[3].bufferIndex = 0

    return vertexDescriptor
}
