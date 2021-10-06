#ifndef BARBU_CORE_WINDOW_H_
#define BARBU_CORE_WINDOW_H_

#include <string_view>

#include "core/display.h"
#include "core/event_callbacks.h"
class Events;

// ----------------------------------------------------------------------------

class AbstractWindow {
 public:
  AbstractWindow()
    : window_w_{0}
    , window_h_{0}
    , screen_w_{0}
    , screen_h_{0}
  {}

  virtual ~AbstractWindow() = default;

  // Create a window for the given display caracterisics.
  virtual bool create(Display const& display, std::string_view title) noexcept = 0;

  // Close the window.
  virtual void close() noexcept = 0;

  // Refresh the window (/ SwapBuffers).
  virtual void flush() noexcept = 0;

  // Process pending events. Return true when the program continue.
  virtual bool poll(Events &events) noexcept = 0; //

  /* API-specific */

  // [OpenGL] Make the context current.
  virtual void makeContextCurrent(bool bSetContext = true) noexcept = 0;
  
  // [Vulkan] Create the surface for the device to render on.
  // virtual void* createVkSurface(void* instance) noexcept = 0;

  /* Setters */

  // Update window title.
  virtual void setTitle(std::string_view title) noexcept = 0;

  // Set the position of the window.
  virtual void setPosition(int x, int y) noexcept = 0;
 
  // Set the window fullscreen status.
  // When enabled the surface is given maximum screen resolution.
  virtual void setFullscreen(bool status) noexcept = 0;

  // Set the visibility for the cursor.
  virtual void showCursor(bool status = true) noexcept = 0;

  // Set the cursor position.
  virtual void setCursorPosition(int x, int y) noexcept = 0;
  
  /* Getters */

  // Return true when the window has focus.
  virtual bool hasFocus() const noexcept = 0; //

  // Return true when the window should close.
  virtual bool shouldClose() const noexcept = 0; // 
  
  // Get the cursor position.
  virtual void getCursorPosition(int *x, int *y) const noexcept = 0; //
  
  // ---------------------
  // [bug prone, use virtual instead]
  inline SurfaceSize width() const { return window_w_; } //
  inline SurfaceSize height() const { return window_h_; } //
  inline SurfaceSize screenWidth() const { return screen_w_; } //
  inline SurfaceSize screenHeight() const { return screen_h_; } //

  /* Graphics API extensions utility */

  struct ExtensionLoaderFuncs_t {
    using IsExtensionSupported_ptr_t = int ((*)(char const*));
    using GetProcAddress_ptr_t       = void (* (*)(char const*))();
    
    IsExtensionSupported_ptr_t isExtensionSupported{nullptr};
    GetProcAddress_ptr_t getProcAddress{nullptr};
  };

  // Return the window manager specific extensions loaders functions.
  virtual ExtensionLoaderFuncs_t getExtensionLoaderFuncs() const noexcept = 0;
  // ---------------------

 protected:
  SurfaceSize window_w_;
  SurfaceSize window_h_;
  SurfaceSize screen_w_;
  SurfaceSize screen_h_;
};

// ----------------------------------------------------------------------------

#include <memory>
using WindowHandle = std::shared_ptr<AbstractWindow>; //

// [idea] use something like :
// static Window* Window::Create(..) { return new SpecializedWindow(); }

// ----------------------------------------------------------------------------

/* Platform specific implementation of the "Window" class. */

#ifdef __ANDROID__
#include "core/impl/android/window.h"
#else
#include "core/impl/desktop/window.h"
#include "core/impl/desktop/symbols.h" //
#endif

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_WINDOW_H_ 
