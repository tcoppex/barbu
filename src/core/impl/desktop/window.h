#ifndef CORE_IMPL_DESKTOP_WINDOW_H_
#define CORE_IMPL_DESKTOP_WINDOW_H_

#include "core/window.h"
struct GLFWwindow;

class Window final : public AbstractWindow {
 public:
  Window() 
    : handle_{nullptr}
    , has_context_{false}
  {}

  ~Window() final;

  bool create(Display const& display, std::string_view title) noexcept final;
  void close() noexcept final;
  void flush() noexcept final;
  bool poll(Events &events) noexcept final;

  void makeContextCurrent(bool bSetContext = true) noexcept final;
  // Surface createDeviceSurface(void* instance) noexcept final { return nullptr; }

  void setTitle(std::string_view title) noexcept final;
  void setPosition(int x, int y) noexcept final;
  void setFullscreen(bool status) noexcept final;
  void showCursor(bool status) noexcept final;
  void setCursorPosition(int x, int y) noexcept final;

  bool hasFocus() const noexcept final;
  bool shouldClose() const noexcept final;
  void getCursorPosition(int *x, int *y) const noexcept final;

  ExtensionLoaderFuncs_t getExtensionLoaderFuncs() const noexcept final;

 private:
  // Limit the window resolution between a min and a max value.
  void setConstraints(SurfaceSize min_w, SurfaceSize min_h, 
                      SurfaceSize max_w, SurfaceSize max_h);

 private:
  GLFWwindow*   handle_;
  bool          has_context_;
};

#endif  // CORE_IMPL_DESKTOP_WINDOW_H_ 