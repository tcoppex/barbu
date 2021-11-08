#include "core/events.h"

#include <algorithm>

#include "core/logger.h"
#include "ui/imgui_wrapper.h"   //<! used for UI events preemption.

// ----------------------------------------------------------------------------

void Events::prepareNextFrame() {
  // Reset per-frame values.
  mouse_moved_        = false;
  mouse_hover_ui_     = false;
  has_resized_        = false;
  last_input_char_    = 0;
  mouse_wheel_delta_  = 0.0f;
  dropped_filenames_.clear();

  // Detect if any mouse buttons are still "Pressed" or "Down".
  mouse_button_down_ = std::any_of(buttons_.cbegin(), buttons_.cend(), [](auto const& btn) {
    auto const state(btn.second);
    return (state == KeyState::Down) || (state == KeyState::Pressed);
  });

  // Update "Pressed" states to "Down", "Released" states to "Up".
  auto const update_state{ [](auto& btn) {
    auto const state(btn.second);
    btn.second = (state == KeyState::Pressed) ? KeyState::Down :
                 (state == KeyState::Released) ? KeyState::Up : state;
  }};
  std::for_each(buttons_.begin(), buttons_.end(), update_state);
  std::for_each(keys_.begin(), keys_.end(), update_state);
}

// ----------------------------------------------------------------------------

/* Bypass events capture when pointer hovers UI. */
#define EVENTS_IMGUI_BYPASS_( code, bMouse, bKeyboard ) \
{ \
  auto &io{ImGui::GetIO()}; \
  mouse_hover_ui_ = io.WantCaptureMouse; \
  { code ;} \
  if ((bMouse) && (bKeyboard)) { return; } \
}

/* Bypass if the UI want the mouse */
#define EVENTS_IMGUI_BYPASS_MOUSE( code ) \
  EVENTS_IMGUI_BYPASS_(code, io.WantCaptureMouse, true)

/* Bypass if the UI want the pointer or the keyboard. */
#define EVENTS_IMGUI_BYPASS_MOUSE_KB( code ) \
  EVENTS_IMGUI_BYPASS_(code, io.WantCaptureMouse, io.WantCaptureKeyboard)

/* Do not bypass. */
#define EVENTS_IMGUI_CONTINUE( code ) \
  EVENTS_IMGUI_BYPASS_(code, false, false)

/* Dispatch event signal to sub callbacks handlers. */
#define EVENTS_DISPATCH_SIGNAL( funcName, ... ) \
  static_assert( std::string_view(#funcName)==__func__, "Incorrect dispatch signal used."); \
  std::for_each(event_callbacks_.begin(), event_callbacks_.end(), [__VA_ARGS__](auto &e){ e->funcName(__VA_ARGS__); })

// ----------------------------------------------------------------------------

void Events::onKeyPressed(KeyCode_t key) {
  EVENTS_IMGUI_CONTINUE( 
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) { 
      io.KeysDown[key] = true; 
    }  
  );

  keys_[key] = KeyState::Pressed;
  key_pressed_.push( key );

  EVENTS_DISPATCH_SIGNAL(onKeyPressed, key);
}

void Events::onKeyReleased(KeyCode_t key) {
  EVENTS_IMGUI_CONTINUE( 
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) { 
      io.KeysDown[key] = false; 
    }  
  );
  
  keys_[key] = KeyState::Released;

  EVENTS_DISPATCH_SIGNAL(onKeyReleased, key);
}

void Events::onInputChar(uint16_t c) {
  EVENTS_IMGUI_BYPASS_MOUSE_KB( 
    if (io.WantCaptureKeyboard) { io.AddInputCharacter(c); }  
  );

  last_input_char_ = c;

  EVENTS_DISPATCH_SIGNAL(onInputChar, c);
}

void Events::onMousePressed(int x, int y, KeyCode_t button) {
  EVENTS_IMGUI_BYPASS_MOUSE( 
    io.MouseDown[button] = true; 
    io.MousePos = ImVec2((float)x, (float)y); 
  );

  buttons_[button] = KeyState::Pressed;

  EVENTS_DISPATCH_SIGNAL(onMousePressed, x, y, button);
}

void Events::onMouseReleased(int x, int y, KeyCode_t button) {
  EVENTS_IMGUI_BYPASS_MOUSE( 
    io.MouseDown[button] = false;
    io.MousePos = ImVec2((float)x, (float)y);
  );

  buttons_[button] = KeyState::Released;

  EVENTS_DISPATCH_SIGNAL(onMouseReleased, x, y, button);
}

void Events::onMouseEntered(int x, int y) {
  EVENTS_DISPATCH_SIGNAL(onMouseEntered, x, y);
}

void Events::onMouseExited(int x, int y) {
  EVENTS_DISPATCH_SIGNAL(onMouseExited, x, y);
}

void Events::onMouseMoved(int x, int y) {
  EVENTS_IMGUI_BYPASS_MOUSE();

  mouse_x_ = x;
  mouse_y_ = y;
  mouse_moved_ = true;

  EVENTS_DISPATCH_SIGNAL(onMouseMoved, x, y);
}

void Events::onMouseDragged(int x, int y, KeyCode_t button) {
  EVENTS_IMGUI_BYPASS_MOUSE( 
    io.MouseDown[button] = true;
    io.MousePos = ImVec2((float)x, (float)y);
  );

  mouse_x_ = x;
  mouse_y_ = y;
  mouse_moved_ = true;
  // (note : the button should already be registered as pressed / down)

  EVENTS_DISPATCH_SIGNAL(onMouseDragged, x, y, button);
}

void Events::onMouseWheel(float dx, float dy) {
  EVENTS_IMGUI_BYPASS_MOUSE( 
    io.MouseWheelH += dx; 
    io.MouseWheel += dy;
  );

  mouse_wheel_delta_ = dy;
  mouse_wheel_ += dy;

  EVENTS_DISPATCH_SIGNAL(onMouseWheel, dx, dy);
}

void Events::onResize(int w, int h) {
  // EVENTS_IMGUI_CONTINUE(
  //   io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
  //   io.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f); //
  // );

  // [beware : downcast from int32 to int16]
  surface_w_ = static_cast<SurfaceSize>(w);
  surface_h_ = static_cast<SurfaceSize>(h);
  has_resized_ = true;

  EVENTS_DISPATCH_SIGNAL(onResize, w, h);
}

void Events::onFilesDropped(int count, char const** paths) {
  for (int i=0; i<count; ++i) {
    dropped_filenames_.push_back(std::string(paths[i]));
  }
  EVENTS_DISPATCH_SIGNAL(onFilesDropped, count, paths);
}

#undef EVENTS_IMGUI_BYPASS
#undef EVENTS_DISPATCH_SIGNAL

// ----------------------------------------------------------------------------

bool Events::buttonDown(KeyCode_t button) const noexcept {
  return checkButtonState( button, [](KeyState state) {
    return (state == KeyState::Pressed) || (state == KeyState::Down); 
  });
} 

bool Events::buttonPressed(KeyCode_t button) const noexcept { 
  return checkButtonState( button, [](KeyState state) {
    return (state == KeyState::Pressed); 
  });
}

bool Events::buttonReleased(KeyCode_t button) const noexcept {
return checkButtonState( button, [](KeyState state) {
    return (state == KeyState::Released); 
  });
}

// ----------------------------------------------------------------------------

bool Events::keyDown(KeyCode_t key) const noexcept {
  return checkKeyState( key, [](KeyState state) {
    return (state == KeyState::Pressed) || (state == KeyState::Down); 
  });
} 

bool Events::keyPressed(KeyCode_t key) const noexcept { 
  return checkKeyState( key, [](KeyState state) {
    return (state == KeyState::Pressed); 
  });
}

bool Events::keyReleased(KeyCode_t key) const noexcept {
return checkKeyState( key, [](KeyState state) {
    return (state == KeyState::Released); 
  });
}

// ----------------------------------------------------------------------------

bool Events::checkButtonState(KeyCode_t button, StatePredicate_t predicate) const noexcept {
  if (auto search = buttons_.find(button); search != buttons_.end()) {
    return predicate(search->second);
  }
  return false;
}

bool Events::checkKeyState(KeyCode_t key, StatePredicate_t predicate) const noexcept {
  if (auto search = keys_.find(key); search != keys_.end()) {
    return predicate(search->second);
  }
  return false;
}

// ----------------------------------------------------------------------------

#if 0

namespace {

void keyboard_cb(GLFWwindow *window, int key, int, int action, int) {

  // [temporary]
  if ((key >= GLFW_KEY_KP_0) && (key <= GLFW_KEY_KP_9)) {
    s_Global.bKeypad |= (action == GLFW_PRESS);
    s_Global.bKeypad &= (action != GLFW_RELEASE);
  }

  // When the UI capture keyboard, don't process it.
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
    io.KeysDown[key] = (action == GLFW_PRESS);

    io.KeyCtrl  = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT]   || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt   = io.KeysDown[GLFW_KEY_LEFT_ALT]     || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER]   || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    
    //return;
  }

  UpdateButton(key, action, GLFW_KEY_LEFT_CONTROL, s_Global.bLeftCtrl);
  UpdateButton(key, action, GLFW_KEY_LEFT_ALT,      s_Global.bLeftAlt);
  UpdateButton(key, action, GLFW_KEY_LEFT_SHIFT,  s_Global.bLeftShift);
}

} // namespace

// ----------------------------------------------------------------------------


#endif
