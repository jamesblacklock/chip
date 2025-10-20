import AppKit

func initWrapper(_ appPath: String, _ vkInst: VkInstance, _ vkSurface: VkSurfaceKHR, _ width: UInt32, _ height: UInt32) -> Bool {
  return appPath.withCString { appPathCString in
    return `init`(CommandLine.argc, CommandLine.unsafeArgv, appPathCString, vkInst, vkSurface, width, height)
  }
}

func handleKey(_ event: NSEvent) {
  if event.isARepeat { return }
  let keyDown = event.type == .keyDown
  // print("macOS code:", event.keyCode)
  // print("ascii:", event.characters?.first?.asciiValue ?? "")
  // print("keyDown:", keyDown)
  switch event.keyCode {
    case 29: set_key_state(Int(KEY_0), keyDown)
    case 18: set_key_state(Int(KEY_1), keyDown)
    case 19: set_key_state(Int(KEY_2), keyDown)
    case 20: set_key_state(Int(KEY_3), keyDown)
    case 21: set_key_state(Int(KEY_4), keyDown)
    case 23: set_key_state(Int(KEY_5), keyDown)
    case 22: set_key_state(Int(KEY_6), keyDown)
    case 26: set_key_state(Int(KEY_7), keyDown)
    case 28: set_key_state(Int(KEY_8), keyDown)
    case 25: set_key_state(Int(KEY_9), keyDown)
    case 0: set_key_state(Int(KEY_A), keyDown)
    case 11: set_key_state(Int(KEY_B), keyDown)
    case 8: set_key_state(Int(KEY_C), keyDown)
    case 2: set_key_state(Int(KEY_D), keyDown)
    case 14: set_key_state(Int(KEY_E), keyDown)
    case 3: set_key_state(Int(KEY_F), keyDown)
    case 5: set_key_state(Int(KEY_G), keyDown)
    case 4: set_key_state(Int(KEY_H), keyDown)
    case 34: set_key_state(Int(KEY_I), keyDown)
    case 38: set_key_state(Int(KEY_J), keyDown)
    case 40: set_key_state(Int(KEY_K), keyDown)
    case 37: set_key_state(Int(KEY_L), keyDown)
    case 46: set_key_state(Int(KEY_M), keyDown)
    case 45: set_key_state(Int(KEY_N), keyDown)
    case 31: set_key_state(Int(KEY_O), keyDown)
    case 35: set_key_state(Int(KEY_P), keyDown)
    case 12: set_key_state(Int(KEY_Q), keyDown)
    case 15: set_key_state(Int(KEY_R), keyDown)
    case 1: set_key_state(Int(KEY_S), keyDown)
    case 17: set_key_state(Int(KEY_T), keyDown)
    case 32: set_key_state(Int(KEY_U), keyDown)
    case 9: set_key_state(Int(KEY_V), keyDown)
    case 13: set_key_state(Int(KEY_W), keyDown)
    case 7: set_key_state(Int(KEY_X), keyDown)
    case 16: set_key_state(Int(KEY_Y), keyDown)
    case 6: set_key_state(Int(KEY_Z), keyDown)
    case 50: set_key_state(Int(KEY_BACKTICK), keyDown)
    case 27: set_key_state(Int(KEY_MINUS), keyDown)
    case 24: set_key_state(Int(KEY_EQUAL), keyDown)
    case 48: set_key_state(Int(KEY_TAB), keyDown)
    case 33: set_key_state(Int(KEY_LBRACK), keyDown)
    case 30: set_key_state(Int(KEY_RBRACK), keyDown)
    case 42: set_key_state(Int(KEY_BACKSLASH), keyDown)
    case 43: set_key_state(Int(KEY_COMMA), keyDown)
    case 47: set_key_state(Int(KEY_DOT), keyDown)
    case 44: set_key_state(Int(KEY_SLASH), keyDown)
    case 49: set_key_state(Int(KEY_SPACE), keyDown)
    case 36: set_key_state(Int(KEY_ENTER), keyDown)
    case 51: set_key_state(Int(KEY_BSP), keyDown)
    case 53: set_key_state(Int(KEY_ESC), keyDown)
    case 123: set_key_state(Int(KEY_LEFT), keyDown)
    case 124: set_key_state(Int(KEY_RIGHT), keyDown)
    case 125: set_key_state(Int(KEY_DOWN), keyDown)
    case 126: set_key_state(Int(KEY_UP), keyDown)
    default: break;
  }
}

class GameView: NSView {
  var keyLshift = false
  var keyRshift = false
  var keyLctrl = false
  var keyRctrl = false
  var keyLmeta = false
  var keyRmeta = false
  var keyLalt = false
  var keyRalt = false
  var mouseX: Float = 0.0;
  var mouseY: Float = 0.0;
  var scrollX: Float = 0.0;
  var scrollY: Float = 0.0;
  var mouseLDown = false;
  var mouseRDown = false;
  var mouseMDown = false;
  var backingScaleFactor: CGFloat = 0;

  override var acceptsFirstResponder: Bool { true } // Allow view to receive key events

  override var wantsLayer: Bool {
    get { true }
    set { }
  }

  override init(frame frameRect: NSRect) {
    super.init(frame: frameRect)
    self.layer = CAMetalLayer()
    self.layer!.backgroundColor = CGColor(gray: 0.0, alpha: 1.0)
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    self.layer = CAMetalLayer()
    self.layer!.backgroundColor = CGColor(gray: 0.0, alpha: 1.0)
  }

  override func scrollWheel(with event: NSEvent) {
    scrollX += Float(event.hasPreciseScrollingDeltas ? event.scrollingDeltaX : -abs(event.scrollingDeltaX) / event.scrollingDeltaX * 10)
    scrollY += Float(event.hasPreciseScrollingDeltas ? event.scrollingDeltaY : -abs(event.scrollingDeltaY) / event.scrollingDeltaY * 10)
    set_scroll_state(scrollX, scrollY)
  }

  override func mouseDown(with event: NSEvent) {
    mouseLDown = true
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func mouseUp(with event: NSEvent) {
    mouseLDown = false
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func rightMouseDown(with event: NSEvent) {
    mouseRDown = true
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func rightMouseUp(with event: NSEvent) {
    mouseRDown = false
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func otherMouseDown(with event: NSEvent) {
    mouseMDown = true
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func otherMouseUp(with event: NSEvent) {
    mouseMDown = false
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func mouseMoved(with event: NSEvent) {
    let pos = event.locationInWindow
    mouseX = Float(pos.x * backingScaleFactor - bounds.width / 2)
    mouseY = Float((bounds.height - pos.y) * backingScaleFactor - bounds.height / 2)
    set_mouse_state(mouseX, mouseY, mouseLDown, mouseRDown, mouseMDown);
  }
  override func mouseDragged(with event: NSEvent) {
    mouseMoved(with: event)
  }
  override func rightMouseDragged(with event: NSEvent) {
    mouseMoved(with: event)
  }
  override func otherMouseDragged(with event: NSEvent) {
    mouseMoved(with: event)
  }
  override func keyDown(with event: NSEvent) {
    handleKey(event)
  }
  override func keyUp(with event: NSEvent) {
    handleKey(event)
  }
  override func flagsChanged(with event: NSEvent) {
    // print(event)
    switch event.keyCode {
      case 56:
        keyLshift = !keyLshift
        set_key_state(Int(KEY_LSHIFT), keyLshift)
      case 60:
        keyRshift = !keyRshift
        set_key_state(Int(KEY_RSHIFT), keyRshift)
      case 59:
        keyLctrl = !keyLctrl
        set_key_state(Int(KEY_LCTRL), keyLctrl)
      case 62:
        keyRctrl = !keyRctrl
        set_key_state(Int(KEY_RCTRL), keyRctrl)
      case 55:
        keyLmeta = !keyLmeta
        set_key_state(Int(KEY_LMETA), keyLmeta)
      case 54:
        keyRmeta = !keyRmeta
        set_key_state(Int(KEY_RMETA), keyRmeta)
      case 58:
        keyLalt = !keyLalt
        set_key_state(Int(KEY_LALT), keyLalt)
      case 61:
        keyRalt = !keyRalt
        set_key_state(Int(KEY_RALT), keyRalt)
      default: break;
    }
  }
}

class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {
  var window: NSWindow!
  var timer: Timer!
  var vkInst: VkInstance!
  var vkSurface: VkSurfaceKHR!
  var lastFrameTime: Double?
  var didInit = false

  func windowDidResize(_ notification: Notification) {
    if let window = notification.object as? NSWindow {
      if let viewBounds = window.contentView?.bounds {
        let scaleFactor = window.backingScaleFactor
        let width = UInt32(viewBounds.width * scaleFactor)
        let height = UInt32(viewBounds.height * scaleFactor)
        window_resized(width, height)
      }
    }
  }

  func applicationDidFinishLaunching(_ aNotification: Notification) {
    let appPath = "\(Bundle.main.bundlePath)/Contents/MacOS/"

    window = NSWindow(
      contentRect: NSRect(x: 0, y: 0, width: 1024, height: 768),
      styleMask: [.titled, .closable, .resizable],
      backing: .buffered,
      defer: false
    )
    window.center()
    window.title = "Game App"
    window.acceptsMouseMovedEvents = true;

    // Set custom view
    let gameView = GameView(frame: window.contentView!.bounds)
    gameView.backingScaleFactor = window.backingScaleFactor
    window.delegate = self
    window.contentView = gameView
    window.makeFirstResponder(gameView) // Ensure view receives key events

    window.makeKeyAndOrderFront(nil)

    NSApp.setActivationPolicy(.regular)
    NSApp.activate(ignoringOtherApps: true)

    // Create Vulkan instance

    "VK_KHR_surface".withCString { ext1 in
    "VK_MVK_macos_surface".withCString { ext2 in
    [UnsafePointer<CChar>?(ext1), UnsafePointer<CChar>?(ext2)].withUnsafeBufferPointer { ppEnabledExtensionNames in

    var instanceInfo = VkInstanceCreateInfo(
      sType: VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      pNext: nil,
      flags: 0,
      pApplicationInfo: nil,
      enabledLayerCount: 0,
      ppEnabledLayerNames: nil,
      enabledExtensionCount: 2,
      ppEnabledExtensionNames: ppEnabledExtensionNames.baseAddress
    )
    var instance: VkInstance?
    if vkCreateInstance(&instanceInfo, nil, &instance) == VK_SUCCESS {
      vkInst = instance
      print("Vulkan instance created")
    } else {
      print("Vulkan instance creation failed")
      return
    }

    }}}

    // Get framebuffer size in pixels
    let viewBounds = gameView.bounds
    let scaleFactor = window.backingScaleFactor
    let fbWidth = UInt32(viewBounds.width * scaleFactor)
    let fbHeight = UInt32(viewBounds.height * scaleFactor)

    // Create macOS surface
    var surface: VkSurfaceKHR?
    var surfaceInfo = VkMacOSSurfaceCreateInfoMVK(
      sType: VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
      pNext: nil,
      flags: 0,
      pView: unsafeBitCast(gameView, to: UnsafeMutableRawPointer.self)
    )
    if vkCreateMacOSSurfaceMVK(vkInst, &surfaceInfo, nil, &surface) == VK_SUCCESS, let surface = surface {
      print("Vulkan surface created")
      vkSurface = surface
      if !initWrapper(appPath, vkInst, vkSurface, fbWidth, fbHeight) {
        NSApp.terminate(self)
        return
      }
      didInit = true
    } else {
      print("Vulkan surface creation failed")
    }

    timer = Timer.scheduledTimer(
      timeInterval: 1.0 / 120.0,
      target: self,
      selector: #selector(gameLoopStep),
      userInfo: nil,
      repeats: true
    )
    timer!.tolerance = 0.001 // Minimize jitter
  }

  @objc func gameLoopStep() {
    if !didInit { return }

    let currentTime = CFAbsoluteTimeGetCurrent()
    let deltaTime = currentTime - (lastFrameTime ?? currentTime) // Time since last frame (seconds)
    lastFrameTime = currentTime
    if !tick(Int(deltaTime * 1000)) {
      NSApp.terminate(self)
    }
  }

  func windowShouldClose(_ sender: NSWindow) -> Bool {
    window_closed()
    return false
  }

  func applicationWillTerminate(_ aNotification: Notification) {
    timer?.invalidate()
    cleanup();
    if let vkInst = vkInst, let vkSurface = vkSurface {
      vkDestroySurfaceKHR(vkInst, vkSurface, nil)
    }
    if let vkInst = vkInst {
      vkDestroyInstance(vkInst, nil)
    }
  }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.run()
