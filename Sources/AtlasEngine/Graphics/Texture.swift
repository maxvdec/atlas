//
//  Texture.swift
//  Atlas
//
//  Created by Max Van den Eynde on 8/7/25.
//

import Metal
import MetalKit

public enum TextureClass: UInt32 {
    case color = 0
    case specular = 1
    case depth = 2
}

public enum TextureFilterMode {
    case linear
    case nearest

    var metal: MTLSamplerMinMagFilter {
        switch self {
        case .linear: return .linear
        case .nearest: return .nearest
        }
    }
}

public enum TextureMipFilterMode {
    case linear
    case nearest
    case notMipMapped

    var metal: MTLSamplerMipFilter {
        switch self {
        case .linear: return .linear
        case .nearest: return .nearest
        case .notMipMapped: return .notMipmapped
        }
    }
}

public enum TextureAddressMode {
    case clampToEdge
    case mirrorClampToEdge
    case mirrorRepeat
    case repeatMode

    var metal: MTLSamplerAddressMode {
        switch self {
        case .clampToEdge: return .clampToEdge
        case .mirrorClampToEdge: return .mirrorClampToEdge
        case .mirrorRepeat: return .mirrorRepeat
        case .repeatMode: return .repeat
        }
    }
}

public struct TextureSampler {
    public var minMag: TextureFilterMode = .linear
    public var mip: TextureMipFilterMode = .linear
    public var address: TextureAddressMode = .repeatMode

    public func makeMetalSampler(device: MTLDevice) -> MTLSamplerState {
        let desc = MTLSamplerDescriptor()
        desc.minFilter = minMag.metal
        desc.magFilter = minMag.metal
        desc.mipFilter = mip.metal
        desc.sAddressMode = address.metal
        desc.tAddressMode = address.metal
        return device.makeSamplerState(descriptor: desc)!
    }
}

public class Texture: Identifiable {
    public var type: TextureClass
    var mtlTexture: MTLTexture!
    public let id: UUID
    public var path: URL?

    public init(type: TextureClass, mtlTexture: MTLTexture, path: URL?) {
        self.type = type
        self.mtlTexture = mtlTexture
        id = UUID()
        self.path = path
    }

    public static func forResource(name: String, type: TextureClass, bundle: Bundle = .main, scaleFactor: Float = 1.0) -> Texture {
        let textureLoader = MTKTextureLoader(device: RenderDispatcher.shared.device)

        do {
            let texture = try textureLoader.newTexture(name: name,
                                                       scaleFactor: CGFloat(scaleFactor),
                                                       bundle: bundle,
                                                       options: [
                                                           .origin: MTKTextureLoader.Origin.bottomLeft,
                                                       ])
            return Texture(type: type, mtlTexture: texture, path: nil)
        } catch {
            fatalError("Could not load texture '\(name)' from asset catalog: \(error.localizedDescription)")
        }
    }

    public static func fromURL(url: URL, type: TextureClass, scaleFactor: Float = 1.0) -> Texture {
        let textureLoader = MTKTextureLoader(device: RenderDispatcher.shared.device)

        do {
            let texture = try textureLoader.newTexture(URL: url, options: [
                .origin: MTKTextureLoader.Origin.bottomLeft,
            ])
            return Texture(type: type, mtlTexture: texture, path: url)
        } catch {
            fatalError("Could not load texture '\(url.absoluteString)': \(error.localizedDescription)")
        }
    }

    static func fromMetal(_ texture: MTLTexture!, type: TextureClass) -> Texture {
        return Texture(type: type, mtlTexture: texture, path: nil)
    }
}

public class FullscreenPresent: Renderable, Identifiable {
    public var texture: Texture
    public var hidden: Bool = true
    public let id: UUID = .init()

    var dispatcher: PostPerform!
    var object: CoreObject!

    public init(texture: Texture) {
        self.texture = texture
        hidden = true

        object = generateCubeObject(size: [2, 2, 2])
        object.shader = FullscreenShader()
        object.initializeCore()

        dispatcher = { renderable, encoder in
            let fullscreen = renderable as! FullscreenPresent
            encoder.setRenderPipelineState(fullscreen.object.pipelineState!)
            encoder.setVertexBuffer(fullscreen.object.vertexBuffer!, offset: 0, index: 0)
            encoder.setFragmentTexture(fullscreen.texture.mtlTexture, index: 0)
            var type = fullscreen.texture.type.rawValue
            encoder.setFragmentBytes(&type, length: MemoryLayout<UInt32>.stride, index: 0)
            encoder.setFragmentSamplerState(fullscreen.object.samplerState!, index: 0)
            encoder.drawIndexedPrimitives(type: .triangle, indexCount: fullscreen.object.indices!.count, indexType: .uint16, indexBuffer: fullscreen.object.indexBuffer!, indexBufferOffset: 0)
        }

        RenderDispatcher.shared.registerPostObject(dispatcher, object: self)
    }

    func isAvailable() -> Bool {
        return !hidden
    }
}
