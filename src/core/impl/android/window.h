#ifndef BARBU_CORE_IMPL_ANDROID_WINDOW_H_
#define BARBU_CORE_IMPL_ANDROID_WINDOW_H_

#include "core/window.h"

class Window final : public AbstractWindow {
 public:
  Window()
    : AbstractWindow{}
  {}

  ~Window() final;

  bool create(Display const& display, std::string_view title) final;
  void close() final;
  void flush() final;

  bool poll(Events& events) final;

  void makeContextCurrent(bool bSetContext) noexcept final;
  
  void setTitle(std::string_view title) final;
  void setPosition(int x, int y) final;
  void setFullscreen(bool status) final;
  void showCursor(bool status) final;
  void setCursorPosition(int x, int y) final;

  bool hasFocus() const final;
  bool shouldClose() const final;
  void getCursorPosition(int *x, int *y) const final;

  ExtensionLoaderFuncs_t getExtensionLoaderFuncs() const noexcept final;
};

#endif  // BARBU_CORE_IMPL_ANDROID_WINDOW_H_ 