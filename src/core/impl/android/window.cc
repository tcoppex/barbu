#include "core/impl/android/window.h"

#include <string_view>

/* -------------------------------------------------------------------------- */

namespace {

static char const* s_Extensions{nullptr};

static
int isExtensionSupported(char const *name) {
  if (s_Extensions) {
    std::string_view name_sv{name};
    for (int i=0; s_Extensions[i]; ++i) {
      if (std::string_view(s_Extensions[i]) == name_sv) {
        return 1;
      }
    }
  }
  return 0;
}

}

/* -------------------------------------------------------------------------- */

Window::~Window() {}

bool Window::create(Display const& display, std::string_view title) {
  return false;
}

void Window::close() {}

void Window::flush() {}

bool Window::poll(Events& events) {
  return true;
}

void makeContextCurrent(bool bSetContext) noexcept {
  
}

void Window::setTitle(std::string_view title) {}

void Window::setPosition(int x, int y) {}

void Window::setFullscreen(bool status) {}

void Window::showCursor(bool status) {}

void Window::setCursorPosition(int x, int y) {}

bool Window::hasFocus() const {
  return true;
}

bool Window::shouldClose() const {
  return false;
}

void Window::getCursorPosition(int *x, int *y) const {}

/* -------------------------------------------------------------------------- */

Window::ExtensionLoaderFuncs_t Window::getExtensionLoaderFuncs() const noexcept {
  //s_Extensions = eglQueryString( egl_display, EGL_EXTENSIONS);
  return ExtensionLoaderFuncs_t{
    nullptr, //isExtensionSupported,
    nullptr //eglGetProcAddress
  };
}

/* -------------------------------------------------------------------------- */
