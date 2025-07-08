//
//  RenderDispatcher.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import Foundation
import Metal
import QuartzCore
import simd

typealias RenderDispatch = (inout CoreObject, inout MTLRenderCommandEncoder) -> Void
typealias FramePerform = (inout Interactive, Double) -> Void

class RenderDispatcher {
    nonisolated(unsafe) static let shared = RenderDispatcher()

    var dispatchers: [RenderDispatch]
    var eachFrame: [FramePerform]
    var objects: [CoreObject]
    var frameObjects: [Interactive]
    var interactives: [Interactive]
    var device: MTLDevice!
    var library: MTLLibrary?
    var aspectRatio: Float = 1.0
    var viewMatrix: simd_float4x4 = makeDefaultViewMatrix()
    var projectionMatrix: simd_float4x4 = makeDefaultProjectionMatrix(aspect: 16.0 / 9.0)
    var lastFrameTime = CACurrentMediaTime()
    var currentScene: RenderScene = RenderScene()

    private init() {
        self.dispatchers = []
        self.objects = []
        self.frameObjects = []
        self.eachFrame = []
        self.interactives = []
        self.device = MTLCreateSystemDefaultDevice()!
    }

    public func dispatchAll(encoder: inout MTLRenderCommandEncoder) {
        for i in 0 ..< dispatchers.count {
            dispatchers[i](&objects[i], &encoder)
        }
    }

    public func performAll(_ delta: Double) {
        for i in 0 ..< frameObjects.count {
            eachFrame[i](&frameObjects[i], delta)
        }
    }

    public func registerObject(coreObject: CoreObject) {
        if let dispatch = coreObject.dispatcher {
            dispatchers.append(dispatch)
        } else {
            print("[Atlas]: Object with ID \(coreObject.id) did not have a dispatcher when registered")
        }
        objects.append(coreObject)
    }

    public func registerFrameObject(_ object: Interactive) {
        eachFrame.append { (object: inout Interactive, time: Double) in
            object.eachFrame(time)
        }

        frameObjects.append(object)
        interactives.append(object)
    }
}
