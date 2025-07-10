import MetalKit
import MetalPerformanceShaders
import SwiftUI

@MainActor
class CoreRenderer: NSObject, MTKViewDelegate {
    var metalView: MTKView
    var metalDevice: MTLDevice!
    var pipelineState: MTLRenderPipelineState!
    var commandQueue: MTLCommandQueue!
    var depthState: MTLDepthStencilState!

    init(metalView: MTKView) {
        self.metalView = metalView

        metalView.device = RenderDispatcher.shared.device!
        self.metalView.isPaused = false
        self.metalView.enableSetNeedsDisplay = false
        self.metalView.depthStencilPixelFormat = .depth32Float
        self.metalView.sampleCount = Atlas.preferences.sampleCount
        self.metalView.clearColor = .init(red: Double(Atlas.preferences.backgroundColor.r), green: Double(Atlas.preferences.backgroundColor.g), blue: Double(Atlas.preferences.backgroundColor.b), alpha: Double(Atlas.preferences.backgroundColor.a))
        self.commandQueue = metalView.device!.makeCommandQueue()
        
        let depthDescriptor = MTLDepthStencilDescriptor()
        depthDescriptor.depthCompareFunction = .less
        depthDescriptor.isDepthWriteEnabled = true
        self.depthState = metalView.device!.makeDepthStencilState(descriptor: depthDescriptor)
    }

    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor else { return }

        let currentTime = CACurrentMediaTime()
        let deltaTime = currentTime - RenderDispatcher.shared.lastFrameTime
        RenderDispatcher.shared.lastFrameTime = currentTime

        var commandBuffer = commandQueue.makeCommandBuffer()
        
        for light in RenderDispatcher.shared.currentScene.lights {
            if light.castsShadows {
                light.makeShadowPass(cmdBuffer: &commandBuffer!, objects: RenderDispatcher.shared.objects)
            }
        }

        var renderEncoder = commandBuffer!.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!
        renderEncoder.setDepthStencilState(depthState)
        renderEncoder.setCullMode(.front)

        RenderDispatcher.shared.performAll(deltaTime)

        RenderDispatcher.shared.dispatchAll(encoder: &renderEncoder)
        renderEncoder.endEncoding()
        
        RenderDispatcher.shared.postDispatchAll(cmdBuffer: commandBuffer!, descriptor: renderPassDescriptor)

        commandBuffer!.present(drawable)
        commandBuffer!.commit()
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
}

class CoreMetalView: MTKView {
    var renderer: CoreRenderer!
    
    private var isMouseCaptured = false
    private var lastMouseLocation: CGPoint = .zero
    private var trackingArea: NSTrackingArea?

    required init(coder: NSCoder) {
        let device = RenderDispatcher.shared.device
        super.init(coder: coder)
        self.device = device
        self.colorPixelFormat = .bgra8Unorm
        self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0)
        self.renderer = CoreRenderer(metalView: self)
        self.delegate = renderer
        setupMouseHandling()
    }

    override init(frame: CGRect, device: MTLDevice? = nil) {
        let useDevice = device ?? RenderDispatcher.shared.device
        super.init(frame: frame, device: useDevice)
        self.colorPixelFormat = .bgra8Unorm
        self.clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0)
        self.renderer = CoreRenderer(metalView: self)
        self.delegate = renderer
        setupMouseHandling()
    }
    
    private func setupMouseHandling() {
        updateTrackingArea()
        
        wantsLayer = true
        layer?.backgroundColor = NSColor.clear.cgColor
    }
    
    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        updateTrackingArea()
    }
    
    private func updateTrackingArea() {
        if let trackingArea = trackingArea {
            removeTrackingArea(trackingArea)
        }
        
        trackingArea = NSTrackingArea(
            rect: bounds,
            options: [
                .activeInActiveApp,
                .mouseMoved,
                .mouseEnteredAndExited,
                .enabledDuringMouseDrag
            ],
            owner: self,
            userInfo: nil
        )
        
        if let trackingArea = trackingArea {
            addTrackingArea(trackingArea)
        }
    }
    
    override var acceptsFirstResponder: Bool {
        return true
    }
    
    override func becomeFirstResponder() -> Bool {
        return true
    }
    
    override func mouseDown(with event: NSEvent) {
        super.mouseDown(with: event)
        
        window?.makeFirstResponder(self)
        
        if !isMouseCaptured {
            captureMouse()
        }
        
        MouseInput.shared.handleMouseClick(button: .left, at: event.locationInWindow)
    }
    
    override func rightMouseDown(with event: NSEvent) {
        super.rightMouseDown(with: event)
        MouseInput.shared.handleMouseClick(button: .right, at: event.locationInWindow)
    }
    
    override func mouseUp(with event: NSEvent) {
        super.mouseUp(with: event)
        MouseInput.shared.handleMouseRelease(button: .left, at: event.locationInWindow)
    }
    
    override func rightMouseUp(with event: NSEvent) {
        super.rightMouseUp(with: event)
        MouseInput.shared.handleMouseRelease(button: .right, at: event.locationInWindow)
    }
    
    override func mouseMoved(with event: NSEvent) {
        super.mouseMoved(with: event)
        handleMouseMovement(event)
    }
    
    override func mouseDragged(with event: NSEvent) {
        super.mouseDragged(with: event)
        handleMouseMovement(event)
    }
    
    override func rightMouseDragged(with event: NSEvent) {
        super.rightMouseDragged(with: event)
        handleMouseMovement(event)
    }
    
    override func scrollWheel(with event: NSEvent) {
        super.scrollWheel(with: event)
        
        let scrollDelta = Float(event.scrollingDeltaY)
        MouseInput.shared.handleMouseScroll(delta: scrollDelta)
    }
    
    private func handleMouseMovement(_ event: NSEvent) {
        let deltaX = Float(event.deltaX)
        let deltaY = Float(event.deltaY)
        
        if abs(deltaX) > 0.01 || abs(deltaY) > 0.01 {
            MouseInput.shared.handleMouseMovement(deltaX: deltaX, deltaY: deltaY)
        }
        
        lastMouseLocation = event.locationInWindow
    }
    
    override func keyDown(with event: NSEvent) {
        super.keyDown(with: event)
    }
    
    func captureMouse() {
        isMouseCaptured = true
        
        CGDisplayHideCursor(CGMainDisplayID())
        CGAssociateMouseAndMouseCursorPosition(0)
    }
    
    func releaseMouse() {
        isMouseCaptured = false
        
        CGAssociateMouseAndMouseCursorPosition(1)
        CGDisplayShowCursor(CGMainDisplayID())
    }
    
    private func recenterMouse() {}
    
    override func mouseEntered(with event: NSEvent) {
        super.mouseEntered(with: event)
    }
    
    override func mouseExited(with event: NSEvent) {
        super.mouseExited(with: event)
        if isMouseCaptured {
            releaseMouse()
        }
    }
}

public class MouseInput {
    public nonisolated(unsafe) static let shared = MouseInput()
    
    public enum MouseButton {
        case left, right, middle
    }
    
    public private(set) var isLeftPressed = false
    public private(set) var isRightPressed = false
    public private(set) var mousePosition: CGPoint = .zero
    public private(set) var mouseDelta: CGPoint = .zero
    
    public var onMouseMovement: ((Float, Float) -> Void)?
    public var onMouseClick: ((MouseButton, CGPoint) -> Void)?
    public var onMouseRelease: ((MouseButton, CGPoint) -> Void)?
    public var onMouseScroll: ((Float) -> Void)?
    
    private init() {}
    
    public func handleMouseMovement(deltaX: Float, deltaY: Float) {
        mouseDelta = CGPoint(x: CGFloat(deltaX), y: CGFloat(deltaY))
        
        for interactive in RenderDispatcher.shared.interactives {
            interactive.onMouseMove([deltaX, deltaY])
        }
        
        onMouseMovement?(deltaX, deltaY)
    }
    
    public func handleMouseClick(button: MouseButton, at position: CGPoint) {
        mousePosition = position
        
        switch button {
        case .left:
            isLeftPressed = true
        case .right:
            isRightPressed = true
        case .middle:
            break
        }
        
        onMouseClick?(button, position)
    }
    
    public func handleMouseRelease(button: MouseButton, at position: CGPoint) {
        mousePosition = position
        
        switch button {
        case .left:
            isLeftPressed = false
        case .right:
            isRightPressed = false
        case .middle:
            break
        }
        
        onMouseRelease?(button, position)
    }
    
    public func handleMouseScroll(delta: Float) {
        for interactive in RenderDispatcher.shared.interactives {
            interactive.onMouseScroll(delta)
        }
        
        onMouseScroll?(delta)
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
        setupMouseInputCallbacks()
    }

    override public func viewDidAppear() {
        super.viewDidAppear()
        view.window?.makeFirstResponder(view)
    }
    
    override public func flagsChanged(with event: NSEvent) {
        let flags = event.modifierFlags.intersection(.deviceIndependentFlagsMask)

        if flags.contains(.shift) {
            Key.addPressedKey(Key.shift)
            Key.addPressedKey(Key.rightShift)
        } else {
            Key.removePressedKey(Key.shift)
            Key.removePressedKey(Key.rightShift)
        }
        
        if flags.contains(.command) {
            Key.addPressedKey(Key.command)
        } else {
            Key.removePressedKey(Key.command)
        }
    }

    private func setupKeyEventHandling() {
        let keyDownMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            self?.handleKeyDown(event)
            return nil
        }

        let keyUpMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyUp) { [weak self] event in
            self?.handleKeyUp(event)
            return nil
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
    
    private func setupMouseInputCallbacks() {
        MouseInput.shared.onMouseMovement = { [weak self] deltaX, deltaY in
            self?.handleCustomMouseMovement(deltaX: deltaX, deltaY: deltaY)
        }
        
        MouseInput.shared.onMouseClick = { [weak self] button, position in
            self?.handleCustomMouseClick(button: button, position: position)
        }
        
        MouseInput.shared.onMouseScroll = { [weak self] delta in
            self?.handleCustomMouseScroll(delta: delta)
        }
    }
    
    private func handleCustomMouseMovement(deltaX: Float, deltaY: Float) {}
    
    private func handleCustomMouseClick(button: MouseInput.MouseButton, position: CGPoint) {}
    
    private func handleCustomMouseScroll(delta: Float) {}

    private func handleKeyDown(_ event: NSEvent) {
        guard let key = Key(rawValue: event.keyCode) else { return }
        
        if event.keyCode == 53 {
            metalView.releaseMouse()
        }

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

    public init(frame: CGRect) {
        self.frame = frame
        
        let aspectRatio = frame.width > 0 && frame.height > 0 ? Float(frame.width / frame.height) : 16.0 / 9.0

        print("MetalRenderView init - frame: \(frame), aspectRatio: \(aspectRatio)")
        RenderDispatcher.shared.aspectRatio = aspectRatio
    }

    public func makeNSViewController(context: Context) -> RenderViewController {
        return RenderViewController(frame: frame)
    }

    public func updateNSViewController(_ nsViewController: RenderViewController, context: Context) {
        nsViewController.setFrame(frame)

        let aspectRatio = frame.width > 0 && frame.height > 0 ? Float(frame.width / frame.height) : 16.0 / 9.0
        RenderDispatcher.shared.aspectRatio = aspectRatio

        print("MetalRenderView update - frame: \(frame), aspectRatio: \(aspectRatio)")
    }
}

public extension MetalRenderView {
    func bodyWithGeometry() -> some View {
        GeometryReader { geometry in
            let frame = geometry.frame(in: .local)
            MetalRenderView(frame: frame)
                .onAppear {
                    let aspectRatio = frame.width > 0 && frame.height > 0 ? Float(frame.width / frame.height) : 16.0 / 9.0
                    RenderDispatcher.shared.aspectRatio = aspectRatio
                    print("MetalRenderView onAppear - frame: \(frame), aspectRatio: \(aspectRatio)")
                }
        }
    }
}

public struct Atlas {
    public nonisolated(unsafe) static var preferences = Atlas()
    
    public var viewportSize: Size2d = [800, 600]
    public var backgroundColor: Color = [0, 0, 0, 1]
    public var sampleCount: Int = 4
    
    private init() {}
}
