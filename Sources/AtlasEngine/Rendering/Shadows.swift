//
//  Shadows.swift
//  Atlas
//
//  Created by Max Van den Eynde on 9/7/25.
//

import Metal
import simd

struct DepthShaderUniforms {
    var lightViewProjectionMatrix: float4x4
}

struct DepthShaderVertex {
    var position: SIMD4<Float>
}

class ShadowRenderer {
    var depthTexture: MTLTexture!
    var device: MTLDevice!
    var shadowPassDescriptor: MTLRenderPassDescriptor!
    var pipelineDescriptor: MTLRenderPipelineDescriptor!
    var pipelineState: MTLRenderPipelineState!
    var depthStencilState: MTLDepthStencilState!

    init(device: MTLDevice) {
        self.device = device

        // First, we make the depth texture
        let shadowMapSize = 1024

        let depthTextureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .depth32Float, width: shadowMapSize, height: shadowMapSize, mipmapped: false)
        depthTextureDescriptor.usage = [.renderTarget, .shaderRead]
        depthTextureDescriptor.storageMode = .private

        depthTexture = device.makeTexture(descriptor: depthTextureDescriptor)!

        // Then, we create the render pass descriptor
        shadowPassDescriptor = MTLRenderPassDescriptor()
        shadowPassDescriptor.depthAttachment.texture = depthTexture
        shadowPassDescriptor.depthAttachment.loadAction = .clear
        shadowPassDescriptor.depthAttachment.storeAction = .store
        shadowPassDescriptor.depthAttachment.clearDepth = 1.0

        shadowPassDescriptor.colorAttachments[0].texture = nil
        shadowPassDescriptor.colorAttachments[0].loadAction = .dontCare
        shadowPassDescriptor.colorAttachments[0].storeAction = .dontCare

        var library = RenderDispatcher.shared.library
        if library == nil {
            RenderDispatcher.shared.library = try! device.makeLibrary(source: allMetalShaders, options: nil)
            library = RenderDispatcher.shared.library
        }

        // Then, the pipeline
        pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = library?.makeFunction(name: "depth_vertex")
        pipelineDescriptor.fragmentFunction = library?.makeFunction(name: "depth_fragment")
        pipelineDescriptor.depthAttachmentPixelFormat = .depth32Float

        let vertexDescriptor = MTLVertexDescriptor()
        vertexDescriptor.attributes[0].format = .float4 // We only use position
        vertexDescriptor.attributes[0].offset = 0
        vertexDescriptor.attributes[0].bufferIndex = 0

        vertexDescriptor.layouts[0].stride = MemoryLayout<DepthShaderVertex>.stride
        vertexDescriptor.layouts[0].stepRate = 1
        vertexDescriptor.layouts[0].stepFunction = .perVertex

        pipelineDescriptor.vertexDescriptor = vertexDescriptor

        pipelineState = try! device.makeRenderPipelineState(descriptor: pipelineDescriptor)

        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.depthCompareFunction = .less
        depthStencilDescriptor.isDepthWriteEnabled = true
        depthStencilState = device.makeDepthStencilState(descriptor: depthStencilDescriptor)!
    }

    public func performShadowPass(cmdBuffer: inout MTLCommandBuffer, objects: [CoreObject], light: Light) {
        let renderEncoder = cmdBuffer.makeRenderCommandEncoder(descriptor: shadowPassDescriptor)!
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setDepthStencilState(depthStencilState)

        renderEncoder.setViewport(MTLViewport(originX: 0, originY: 0, width: 1024, height: 1024, znear: 0.0, zfar: 1.0))

        for object in objects {
            if object.shader.type == .basicShader || object.shader.type == .fullscreenShader {
                continue
            }
            var lightVP = light.getLightViewProjectionMatrix()
            print(lightVP)
            renderEncoder.setVertexBytes(&lightVP, length: MemoryLayout<float4x4>.stride, index: 2)
            renderEncoder.setVertexBuffer(object.depthVertexBuffer!, offset: 0, index: 0)
            var objectModel = object.makeModelMatrix()
            renderEncoder.setVertexBytes(&objectModel, length: MemoryLayout<float4x4>.stride, index: 1)
            if object.indices != nil {
                renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: object.indices!.count, indexType: .uint16, indexBuffer: object.indexBuffer!, indexBufferOffset: 0)
            } else {
                renderEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: object.vertices.count)
            }
        }
        renderEncoder.endEncoding()
    }
}
