//
//  ShadowRenderer.swift
//  Atlas
//
//  Created by Max Van den Eynde on 10/7/25.
//

import Metal
import simd

struct DepthVertex {
    var position: SIMD3<Float>
}

struct ShadowRenderer {
    var texture: MTLTexture!
    var renderPassDescriptor: MTLRenderPassDescriptor!
    var depthStencilState: MTLDepthStencilState!
    var pipeline: MTLRenderPipelineState!

    init(device: MTLDevice) {
        let shadowMapSize = 1024
        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .depth32Float, width: shadowMapSize, height: shadowMapSize, mipmapped: false)
        textureDescriptor.usage = [.shaderRead, .renderTarget]
        textureDescriptor.storageMode = .private
        texture = device.makeTexture(descriptor: textureDescriptor)!

        renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.depthAttachment.texture = texture
        renderPassDescriptor.depthAttachment.loadAction = .clear
        renderPassDescriptor.depthAttachment.storeAction = .store
        renderPassDescriptor.depthAttachment.clearDepth = 1.0

        let depthDesc = MTLDepthStencilDescriptor()
        depthDesc.depthCompareFunction = .less
        depthDesc.isDepthWriteEnabled = true

        depthStencilState = device.makeDepthStencilState(descriptor: depthDesc)!

        if RenderDispatcher.shared.library == nil {
            RenderDispatcher.shared.library = try! device.makeLibrary(source: allMetalShaders, options: nil)
            if RenderDispatcher.shared.library == nil {
                fatalError("Could not create library")
            }
        }

        let library = RenderDispatcher.shared.library!
        let vertex = library.makeFunction(name: "depth_vertex")!

        let pipelineDesc = MTLRenderPipelineDescriptor()
        pipelineDesc.vertexFunction = vertex
        pipelineDesc.fragmentFunction = nil

        pipelineDesc.colorAttachments[0].pixelFormat = .invalid
        pipelineDesc.depthAttachmentPixelFormat = .depth32Float

        let vertexDesc = MTLVertexDescriptor()
        vertexDesc.attributes[0].format = .float3
        vertexDesc.attributes[0].bufferIndex = 0
        vertexDesc.attributes[0].offset = 0

        vertexDesc.layouts[0].stepRate = 1
        vertexDesc.layouts[0].stepFunction = .perVertex
        vertexDesc.layouts[0].stride = MemoryLayout<DepthVertex>.stride

        pipelineDesc.vertexDescriptor = vertexDesc

        pipeline = try! device.makeRenderPipelineState(descriptor: pipelineDesc)
    }

    func executeShadowPass(commandBuffer: MTLCommandBuffer, objects: [CoreObject], light: Light) {
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: light.shadowRenderer.renderPassDescriptor)!
        renderEncoder.setRenderPipelineState(light.shadowRenderer.pipeline)
        renderEncoder.setDepthStencilState(light.shadowRenderer.depthStencilState)
        var lightVP = light.getLightVPMatrix()
        renderEncoder.setVertexBytes(&lightVP, length: MemoryLayout<simd_float4x4>.stride, index: 1)

        for object in objects {
            renderEncoder.setVertexBuffer(object.depthBuffer, offset: 0, index: 0)
            if object.useIndexedDrawing {
                renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: object.indices!.count, indexType: .uint16, indexBuffer: object.indexBuffer!, indexBufferOffset: 0)
            } else {
                renderEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: object.vertices.count)
            }
        }
        renderEncoder.endEncoding()
    }
}
