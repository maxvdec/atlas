//
//  Camera.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

public class Camera: Interactive {
    public func eachFrame(_ self: inout any Interactive) {}

    public init() {
        RenderDispatcher.shared.registerFrameObject(self)
    }
}
