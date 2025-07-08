//
//  RenderDispatcher.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import Foundation
import Metal

typealias RenderDispatch = (inout CoreObject, inout MTLRenderCommandEncoder) -> Void
typealias FramePerform = (inout Interactive) -> Void

class RenderDispatcher {
    nonisolated(unsafe) static let shared = RenderDispatcher()

    var dispatchers: [RenderDispatch]
    var eachFrame: [FramePerform]
    var objects: [CoreObject]
    var frameObjects: [Interactive]
    var device: MTLDevice!
    var library: MTLLibrary?

    private init() {
        self.dispatchers = []
        self.objects = []
        self.frameObjects = []
        self.eachFrame = []
        self.device = MTLCreateSystemDefaultDevice()!
    }

    public func dispatchAll(encoder: inout MTLRenderCommandEncoder) {
        for i in 0 ..< dispatchers.count {
            dispatchers[i](&objects[i], &encoder)
        }
    }

    public func performAll() {
        for i in 0 ..< frameObjects.count {
            eachFrame[i](&frameObjects[i])
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
        eachFrame.append { object in
            object.eachFrame(&object)
        }
        frameObjects.append(object)
    }
}
