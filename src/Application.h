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
  void draw() final;

  void onInputChar(uint16_t c) final;
  void onResize(int w, int h) final;
  // void onKeyPressed(KeyCode_t key) final;
  // void onKeyReleased(KeyCode_t key) final;
  // void onMousePressed(int x, int y, KeyCode_t button) final;
  // void onMouseReleased(int x, int y, KeyCode_t button) final;
  // void onMouseEntered(int x, int y) final;
  // void onMouseExited(int x, int y) final;
  // void onMouseMoved(int x, int y) final;
  // void onMouseDragged(int x, int y, KeyCode_t button) final;
  // void onMouseWheel(float dx, float dy) final;

 private:
  static constexpr bool kCentroid{ true };
  static constexpr bool kSmooth{ true };
  static constexpr float kPi{ glm::pi<float>() };
  static constexpr float kDefaultRefocusDistance{ 3.50f };
  static constexpr float kRefocusDistanceScaling{ 1.25f };

  void refocusCamera(bool bCentroid, bool bSmooth, EntityHandle new_focus = nullptr);

  ArcBallController arcball_;       //< Controller for the camera.
  EntityHandle focus_;              //< Entity to focus on.
  bool bRefocus_ = false;           //< When true, refocus the camera.
};


// ----------------------------------------------------------------------------

#endif  // BARBU_APPLICATION_H_
