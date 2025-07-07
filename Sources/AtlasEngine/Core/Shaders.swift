//
//  Shaders.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import Metal
import MetalKit

protocol CoreShader {
    func makePipeline(device: MTLDevice, library: MTLLibrary) -> MTLRenderPipelineState
}

class BasicShader: CoreShader {
    public func makePipeline(device: any MTLDevice) -> any MTLRenderPipelineState {
        if RenderDispatcher.shared.library == nil {
            RenderDispatcher.shared.library = try! device.makeLibrary(source: allMetalShaders, options: nil)
            if RenderDispatcher.shared.library == nil {
                fatalError("Could not create library")
            }
        }
        
        let library = RenderDispatcher.shared.library!
        let vertex = library.makeFunction(name: "vertex_main")!
        let fragment = library.makeFunction(name: "fragment_main")!
    }
}
