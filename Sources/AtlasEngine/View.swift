import MetalKit
import SwiftUI

@MainActor
class CoreRenderer: NSObject, MTKViewDelegate {
    var metalView: MTKView
    var metalDevice: MTLDevice!
    var pipelineState: MTLRenderPipelineState!
    var commandQueue: MTLCommandQueue!

    init(metalView: MTKView) {
        self.metalView = metalView

        metalView.device = RenderDispatcher.shared.device!
        self.metalView.isPaused = false
        self.metalView.enableSetNeedsDisplay = false
        self.commandQueue = metalView.device!.makeCommandQueue()
    }

    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor else { return }

        let commandBuffer = commandQueue.makeCommandBuffer()

        var renderEncoder = commandBuffer!.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!

        RenderDispatcher.shared.performAll()

        RenderDispatcher.shared.dispatchAll(encoder: &renderEncoder)

        renderEncoder.endEncoding()
        commandBuffer!.present(drawable)
        commandBuffer!.commit()
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
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
    private var eventMonitors: [Any] = []

    init(frame: CGRect) {
        self.initialFrame = frame
        super.init(nibName: nil, bundle: nil)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override public func loadView() {
        let metalView = CoreMetalView(frame: initialFrame)
        self.metalView = metalView
        view = metalView

        metalView.canBecomeKeyView = true
    }

    override public func viewDidLoad() {
        super.viewDidLoad()
        setupKeyEventHandling()
    }

    override public func viewDidAppear() {
        super.viewDidAppear()
        view.window?.makeFirstResponder(view)
    }

    private func setupKeyEventHandling() {
        // Monitor key down events
        let keyDownMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            self?.handleKeyDown(event)
            return event
        }

        let keyUpMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyUp) { [weak self] event in
            self?.handleKeyUp(event)
            return event
        }

        if let keyDownMonitor = keyDownMonitor {
            eventMonitors.append(keyDownMonitor)
        }
        if let keyUpMonitor = keyUpMonitor {
            eventMonitors.append(keyUpMonitor)
        }

        NotificationCenter.default.addObserver(
            self,
            selector: #selector(windowDidResignKey),
            name: NSWindow.didResignKeyNotification,
            object: nil
        )
    }

    private func handleKeyDown(_ event: NSEvent) {
        guard let key = Key(rawValue: event.keyCode) else { return }

        if !event.isARepeat {
            Key.addPressedKey(key)
        }
    }

    private func handleKeyUp(_ event: NSEvent) {
        guard let key = Key(rawValue: event.keyCode) else { return }
        Key.removePressedKey(key)
    }

    @objc private func windowDidResignKey(_ notification: Notification) {
        Key.clearAllPressedKeys()
    }

    func setFrame(_ frame: CGRect) {
        view.frame = frame
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
