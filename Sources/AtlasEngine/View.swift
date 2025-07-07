import MetalKit
import SwiftUI

class CoreRenderer: NSObject, MTKViewDelegate {
    var metalView: MTKView
    var metalDevice: MTLDevice!
    var pipelineState: MTLRenderPipelineState!
    
    init(metalView: MTKView) {
        self.metalView = metalView
    }
    
    func draw(in view: MTKView) {
        
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        
    }
}

class CoreMetalView: MTKView {
    var renderer: CoreRenderer!

    required init(coder: NSCoder) {
        let device = RenderDispatcher.shared.device
        super.init(coder: coder)
        self.device = device
        self.colorPixelFormat = .bgra8Unorm
        self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0)
        self.renderer = CoreRenderer(metalView: self)
        self.delegate = renderer
    }

    override init(frame: CGRect, device: MTLDevice? = nil) {
        let useDevice = device ?? RenderDispatcher.shared.device
        super.init(frame: frame, device: useDevice)
        self.colorPixelFormat = .bgra8Unorm
        self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0)
        self.renderer = CoreRenderer(metalView: self)
        self.delegate = renderer
    }
}

public class RenderViewController: NSViewController {
    private var metalView: CoreMetalView!
    private var initialFrame: CGRect
    
    init(frame: CGRect) {
        self.initialFrame = frame
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    public override func loadView() {
        let metalView = CoreMetalView(frame: initialFrame)
        self.metalView = metalView
        self.view = metalView
    }
    
    func setFrame(_ frame: CGRect) {
        self.view.frame = frame
        metalView.frame = frame
    }
}

public struct MetalRenderView: NSViewControllerRepresentable {
    private var frame: CGRect = .zero
    
    public init(frame: CGRect = .zero) {
        self.frame = frame
    }

    public func makeNSViewController(context: Context) -> RenderViewController {
        RenderViewController(frame: frame)
    }
    
    public func updateNSViewController(_ nsViewController: RenderViewController, context: Context) {
        nsViewController.setFrame(frame)
    }
}

public extension MetalRenderView {
    func bodyWithGeometry() -> some View {
        GeometryReader { geometry in
            MetalRenderView(frame: geometry.frame(in: .local))
        }
    }
}
