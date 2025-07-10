//
//  ShadowRenderer.swift
//  Atlas
//
//  Created by Max Van den Eynde on 10/7/25.
//

import Metal

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

        pipeline = try! device.makeRenderPipelineState(descriptor: pipelineDesc)
    }

    func executeShadowPass(commandBuffer: MTLCommandBuffer, objects: [CoreObject], light: Light) {}
}
