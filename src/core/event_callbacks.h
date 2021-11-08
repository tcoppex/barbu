#ifndef CORE_EVENT_CALLBACKS_H_
#define CORE_EVENT_CALLBACKS_H_

using KeyCode_t = uint16_t;

//
// Define event signal callbacks handled by the window.
//
struct EventCallbacks {  
  virtual ~EventCallbacks() = default;

  virtual void onKeyPressed(KeyCode_t key) {}

  virtual void onKeyReleased(KeyCode_t key) {}

  virtual void onInputChar(uint16_t c) {}

  virtual void onMousePressed(int x, int y, KeyCode_t button) {}

  virtual void onMouseReleased(int x, int y, KeyCode_t button) {}

  virtual void onMouseEntered(int x, int y) {}

  virtual void onMouseExited(int x, int y) {}

  virtual void onMouseMoved(int x, int y) {}

  virtual void onMouseDragged(int x, int y, KeyCode_t button) {}

  virtual void onMouseWheel(float dx, float dy) {}

  virtual void onResize(int w, int h) {}

  virtual void onFilesDropped(int count, char const** paths) {} //
};

using EventCallbacksPtr = EventCallbacks*;

#endif  // CORE_EVENT_CALLBACKS_H_ 
