#ifndef BARBU_CORE_EVENTS_H_
#define BARBU_CORE_EVENTS_H_

#include <functional>
#include <stack>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/singleton.h"        //< inheritance.
#include "core/event_callbacks.h"   //< inheritance.
#include "core/display.h"           //< SurfaceSize.
// #include "core/window.h"            //< Window::poll friendship.

// ----------------------------------------------------------------------------

/**
 * Manage and post-process captured event signals then dispatch them to
 * sub event handlers.
 */
class Events final : public Singleton<Events>
                   , private EventCallbacks
{
 public:
  /* Different states for a key / button :
   *  - "Pressed" and "Released" stay in this state for only one frame.
   *  - "Pressed" is also considered "Down",
   *  - "Released" is also considered "Up".
   */
  enum class KeyState : uint8_t {
    Up,
    Down,
    Pressed, 
    Released
  };

  Events()
    : mouse_moved_(false)
    , mouse_button_down_(false)
    , mouse_hover_ui_(false)
    , has_resized_(false)
    , surface_w_(0)
    , surface_h_(0)
    , mouse_x_(0)
    , mouse_y_(0)
    , mouse_wheel_(0.0f)
    , mouse_wheel_delta_(0.0f)
    , last_input_char_(0)
  {}

  /* Update internal data for the next frame. */
  void prepareNextFrame();

  /* Register event callbacks to dispatch signals to. */
  void registerCallbacks(EventCallbacksPtr eh) {
    event_callbacks_.insert(eh);
  }

  /* Getters */

  inline bool mouseMoved() const noexcept { return mouse_moved_; }
  inline bool hasButtonDown() const noexcept { return mouse_button_down_; }
  inline bool mouseHoverUI() const noexcept { return mouse_hover_ui_; }
  inline bool hasResized() const noexcept { return has_resized_; }

  inline SurfaceSize surface_width() const noexcept { return surface_w_; };
  inline SurfaceSize surface_height() const noexcept { return surface_h_; };

  inline int mouseX() const noexcept { return mouse_x_; }
  inline int mouseY() const noexcept { return mouse_y_; }

  inline float wheel() const noexcept { return mouse_wheel_; }
  inline float wheelDelta() const noexcept { return mouse_wheel_delta_; }

  bool buttonDown(KeyCode_t button) const noexcept;
  bool buttonPressed(KeyCode_t button) const noexcept;
  bool buttonReleased(KeyCode_t button) const noexcept;
  
  bool keyDown(KeyCode_t key) const noexcept;
  bool keyPressed(KeyCode_t key) const noexcept;
  bool keyReleased(KeyCode_t key) const noexcept;

  /* Return the last user's keystroke. */
  inline KeyCode_t lastKeyDown() const noexcept {
    return key_pressed_.empty() ? -1 : key_pressed_.top(); //
  }

  /* Return the last frame input char, if any. */
  inline uint16_t lastInputChar() const noexcept {
    return last_input_char_;
  }

  /* Return the last frame dropped filenames, if any. */
  inline std::vector<std::string> const& droppedFilenames() const noexcept {
    return dropped_filenames_;
  }

 //private:
  /* EventCallbacks override */

  void onKeyPressed(KeyCode_t key) final;
  void onKeyReleased(KeyCode_t key) final;
  void onInputChar(uint16_t c) final;
  void onMousePressed(int x, int y, KeyCode_t button) final;
  void onMouseReleased(int x, int y, KeyCode_t button) final;
  void onMouseEntered(int x, int y) final;
  void onMouseExited(int x, int y) final;
  void onMouseMoved(int x, int y) final;
  void onMouseDragged(int x, int y, KeyCode_t button) final;
  void onMouseWheel(float dx, float dy) final;
  void onResize(int w, int h) final;
  void onFilesDropped(int count, char const** paths) final;

  //friend bool Window::poll(Events&);

 private:
  using KeyMap_t = std::unordered_map<KeyCode_t, KeyState>;
  using StatePredicate_t = std::function<bool(KeyState)> const&;

  /* Check if a key state match a state predicate. */
  bool checkButtonState(KeyCode_t button, StatePredicate_t predicate) const noexcept;
  bool checkKeyState(KeyCode_t key, StatePredicate_t predicate) const noexcept;

  // Per-frame state.
  bool mouse_moved_;
  bool mouse_button_down_;
  bool mouse_hover_ui_;
  bool has_resized_;

  // Window
  SurfaceSize surface_w_;
  SurfaceSize surface_h_;

  // Mouse.
  float mouse_x_;
  float mouse_y_;
  float mouse_wheel_;
  float mouse_wheel_delta_;

  // Buttons.
  KeyMap_t buttons_;
  
  // Keys.
  KeyMap_t keys_;
  std::stack<KeyCode_t> key_pressed_;

  // Char input.
  uint16_t last_input_char_;

  // Drag-n-dropped filenames.
  std::vector<std::string> dropped_filenames_;

  // Registered events callbacks.
  std::set<EventCallbacksPtr> event_callbacks_;

 private:
  friend class Singleton<Events>;
};

// ----------------------------------------------------------------------------

#endif // BARBU_CORE_EVENTS_H_
