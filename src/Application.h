#ifndef BARBU_APPLICATION_H_
#define BARBU_APPLICATION_H_

#include "core/app.h"
#include "utils/arcball_controller.h"
#include "ecs/entity-fwd.h"

// ----------------------------------------------------------------------------

class Application final : public App {
 public:
  Application() = default;
  ~Application() {}

  void setup() final;
  
  void update() final;

  /* -- Events Capture -- */

  // void onKeyPressed(KeyCode_t key) final;
  // void onKeyReleased(KeyCode_t key) final;
  void onInputChar(uint16_t c) final;
  // void onMousePressed(int x, int y, KeyCode_t button) final;
  // void onMouseReleased(int x, int y, KeyCode_t button) final;
  // void onMouseEntered(int x, int y) final;
  // void onMouseExited(int x, int y) final;
  // void onMouseMoved(int x, int y) final;
  // void onMouseDragged(int x, int y, KeyCode_t button) final;
  // void onMouseWheel(float dx, float dy) final;
  void onResize(int w, int h) final;

 private:
  void refocus_camera(bool bCentroid, bool bNoSmooth, EntityHandle new_focus = nullptr);

  ArcBallController arcball_controller_;
  EntityHandle focus_;
  bool bRefocus_ = false;
};


// ----------------------------------------------------------------------------

#endif  // BARBU_APPLICATION_H_
