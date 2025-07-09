//
//  Shadows.swift
//  Atlas
//
//  Created by Max Van den Eynde on 9/7/25.
//

import Metal

class ShadowRenderer {
    var depthTexture: MTLTexture!
    var device: MTLDevice!
    var renderPassDescriptor: MTLRenderPassDescriptor!
    var pipelineDescritpro: MTLRenderPipelineDescriptor!

    init(device: MTLDevice) {
        self.device = device

        // First, we make the depth texture
        let shadowMapSize = 1024
    }
}
