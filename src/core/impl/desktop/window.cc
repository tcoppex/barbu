#include "core/impl/desktop/window.h"

// window manager
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include "core/logger.h"
#include "core/events.h"

/* -------------------------------------------------------------------------- */

namespace {

void InitializeEventsCallbacks(GLFWwindow *handle) noexcept {
  // Keyboard.
  glfwSetKeyCallback(handle, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto &events = Events::Get();
    
    if (action == GLFW_PRESS) {
      events.onKeyPressed(key);
    } else if (action == GLFW_RELEASE) {
      events.onKeyReleased(key);
    }
  });

  // Input Char.
  glfwSetCharCallback(handle, [](GLFWwindow*, unsigned int c) {
    if ((c > 0) && (c < 0x10000)) {
      Events::Get().onInputChar(static_cast<uint16_t>(c));
    }
  });

  // Mouse buttons.
  glfwSetMouseButtonCallback(handle, [](GLFWwindow* window, int button, int action, int mods) {
    auto &events = Events::Get();
    auto const mouse_x = events.mouseX();
    auto const mouse_y = events.mouseY();
    
    if (action == GLFW_PRESS) {
      events.onMousePressed(mouse_x, mouse_y, button);
    } else if (action == GLFW_RELEASE) {
      events.onMouseReleased(mouse_x, mouse_y, button);
    }
  });

  // Mouse Enter / Exit.
  glfwSetCursorEnterCallback(handle, [](GLFWwindow* window, int entered) {
    auto &events = Events::Get();
    auto const mouse_x = events.mouseX();
    auto const mouse_y = events.mouseY();

    entered ? events.onMouseEntered(mouse_x, mouse_y)
            : events.onMouseExited(mouse_x, mouse_y)
            ;
  });

  // Mouse motion.
  glfwSetCursorPosCallback(handle, [](GLFWwindow* window, double x, double y) {
    auto &events = Events::Get();
    bool const bAnyButtonDown = events.hasButtonDown();

    auto const px = static_cast<int>(x);
    auto const py = static_cast<int>(y);

    bAnyButtonDown ? events.onMouseDragged(px, py, 0/**/) // FIXME
                   : events.onMouseMoved(px, py)
                   ;
  });

  // Mouse wheel.
  glfwSetScrollCallback(handle, [](GLFWwindow* window, double dx, double dy) {
    Events::Get().onMouseWheel(
      static_cast<float>(dx), 
      static_cast<float>(dy)
    );
  });

  // Drag-n-drop.
  glfwSetDropCallback(handle, [](GLFWwindow* window, int count, char const** paths) {
    // LOG_WARNING( "Window signal glfwSetDropCallback is not implemented." );
    Events::Get().onFilesDropped(count, paths);
  });

  // Window resize.
  glfwSetWindowSizeCallback(handle, [](GLFWwindow *window, int w, int h) {
    LOG_WARNING( "Window signal glfwSetWindowSizeCallback is not implemented." );
    // Events::Get().onResize(w, h);
  });

  // Framebuffer resize.
  glfwSetFramebufferSizeCallback(handle, [](GLFWwindow *window, int w, int h) {
    Events::Get().onResize(w, h);
  });
}

} // namespace

/* -------------------------------------------------------------------------- */

Window::~Window() {
  if (handle_) {
    glfwDestroyWindow(handle_);
    handle_ = nullptr;
  }
}

bool Window::create(Display const& display, std::string_view title) noexcept {
  // Initialize Window Management API.
  if (!glfwInit()) {
    LOG_ERROR( "GLFW failed to be initialized." );
    return false;
  }
  atexit(glfwTerminate);

  // Flags.
  bool const bDecorated  = display.flags & DisplayFlags_Decorated;
  bool const bResizable  = display.flags & DisplayFlags_Resizable;
  bool const bFullscreen = display.flags & DisplayFlags_FullScreen;

  // Window hints.
  glfwWindowHint( GLFW_DECORATED,               bDecorated);
  glfwWindowHint( GLFW_RESIZABLE,               bResizable);
  glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
  glfwWindowHint( GLFW_FOCUS_ON_SHOW,           GLFW_TRUE);
  glfwWindowHint( GLFW_VISIBLE,                 GLFW_TRUE);

  // Framebuffers hints.
  glfwWindowHint( GLFW_SRGB_CAPABLE,            GLFW_TRUE);
  glfwWindowHint( GLFW_DOUBLEBUFFER,            GLFW_TRUE);
  glfwWindowHint( GLFW_SAMPLES,                 display.msaa_samples);
  
  // Context hints.
  if (GraphicsAPI::kOpenGL == display.api)
  {
    if (ShaderModel::GL_CORE_42 == display.shader_model) {
      glfwWindowHint( GLFW_CLIENT_API,              GLFW_OPENGL_API);
      glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR,   4);
      glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR,   2);
      glfwWindowHint( GLFW_OPENGL_PROFILE,          GLFW_OPENGL_CORE_PROFILE);
      glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT,   GLFW_TRUE);
    } else {
      glfwWindowHint( GLFW_CLIENT_API,              GLFW_OPENGL_ES_API);
      glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR,   3);
      glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR,   2);
    }
    glfwWindowHint( GLFW_CONTEXT_CREATION_API,      GLFW_EGL_CONTEXT_API); // GLFW_NATIVE_CONTEXT_API
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT,      GLFW_FALSE); //
    has_context_ = true;
  } else {
    // Vulkan-like APIs need no context.
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API);
  }

  // Compute window resolution based-off the main monitor's resolution.
  {
    GLFWvidmode const* mode{ glfwGetVideoMode(glfwGetPrimaryMonitor()) };
    screen_w_ = mode->width;
    screen_h_ = mode->height;
    window_w_ = bFullscreen ? screen_w_ : ClampSurfaceSize( display.width, screen_w_);
    window_h_ = bFullscreen ? screen_h_ : ClampSurfaceSize( display.height, screen_h_);
  }

  // Create the window.
  handle_ = glfwCreateWindow( window_w_, window_h_, title.data(), nullptr, nullptr);
  if (!handle_) {
    LOG_ERROR( "The window creation failed." );
    return false;
  }

  // Setup events callbacks.
  InitializeEventsCallbacks(handle_);

  if (has_context_) {
    // Make the window's context current.
    makeContextCurrent();

    // Disable v-sync to not be FPS-bounded. This might cause flickering on resize.
    glfwSwapInterval(0);
  }

  // Do not constraints aspect ratio.
  glfwSetWindowAspectRatio(handle_, GLFW_DONT_CARE, GLFW_DONT_CARE);

  // Check resolution.
  {
    // Framebuffer size might change depending on the monitor !
    int32_t w{0};
    int32_t h{0};
    glfwGetFramebufferSize(handle_, &w, &h);

    // Simulate an initial "onResize" event..
    Events::Get().onResize(w, h); //
  }

  return true;
}

void Window::close() noexcept {
  glfwSetWindowShouldClose(handle_, GLFW_TRUE); //
}

void Window::flush() noexcept {
  glfwSwapBuffers(handle_);
}

bool Window::poll(Events &_events) noexcept {
  // Capture and dispatch events.
  glfwPollEvents();

  // Capture generic events.
  // [ Better alternative : register a default event_callbacks structure ]

  // Close when hitting escape.
  if (_events.keyPressed(GLFW_KEY_ESCAPE)) {
    close();
  }

  if (_events.hasResized()) {
    window_w_ = _events.surface_width();
    window_h_ = _events.surface_height(); 
  }

  return !shouldClose();
}

void Window::makeContextCurrent(bool bSetContext) noexcept {
  assert( has_context_ );
  glfwMakeContextCurrent( bSetContext ? handle_ : nullptr );
}

void Window::setTitle(std::string_view title) noexcept {
  glfwSetWindowTitle( handle_, title.data());
}

void Window::setPosition(int x, int y) noexcept {
  glfwSetWindowPos( handle_, x, y);
}

void Window::setFullscreen(bool status) noexcept {
  LOG_WARNING( "Window::setFullscreen is not implemented." );
}

void Window::showCursor(bool status) noexcept {
  if (glfwGetInputMode( handle_, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
    glfwSetInputMode( handle_, GLFW_CURSOR, status ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  }
}

void Window::setCursorPosition(int x, int y) noexcept {
  glfwSetCursorPos( handle_, static_cast<double>(x), static_cast<double>(y));
}

bool Window::hasFocus() const noexcept {
  return glfwGetWindowAttrib( handle_, GLFW_FOCUSED);
}

bool Window::shouldClose() const noexcept {
  return GLFW_TRUE == glfwWindowShouldClose( handle_ );
}

void Window::getCursorPosition(int *x, int *y) const noexcept {
  double fx{0.0}, fy{0.0};
  glfwGetCursorPos( handle_, &fx, &fy);
  *x = static_cast<int>(fx);
  *y = static_cast<int>(fy);
}

/* -------------------------------------------------------------------------- */

Window::ExtensionLoaderFuncs_t Window::getExtensionLoaderFuncs() const noexcept {
  assert( has_context_ );
  return ExtensionLoaderFuncs_t{
    glfwExtensionSupported, 
    glfwGetProcAddress
  };
}

/* -------------------------------------------------------------------------- */

void Window::setConstraints(SurfaceSize min_w, SurfaceSize min_h, 
                            SurfaceSize max_w, SurfaceSize max_h)
{
  glfwSetWindowSizeLimits( handle_, min_w, min_h, max_w, max_h);
}

/* -------------------------------------------------------------------------- */
