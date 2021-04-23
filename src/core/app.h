#ifndef BARBU_CORE_APP_H_
#define BARBU_CORE_APP_H_

// ----------------------------------------------------------------------------

#include <chrono>
#include "glm/mat4x4.hpp"

#include "core/camera.h"
#include "fx/postprocess/postprocess.h"
#include "ui/ui_controller.h"
#include "utils/arcball_controller.h"
#include "utils/gizmo.h"

struct GLFWwindow;

namespace views {
  class Main;
}

// ----------------------------------------------------------------------------

// Bitflags to filter which scene pass to render.
enum SceneFilterBit : uint32_t {
  SCENE_EVERYTHING        = 0x7fffffff,
  SCENE_NONE              = 0,

  SCENE_SKYBOX_BIT        = 1 << 0,
  SCENE_SOLID_BIT         = 1 << 1,
  SCENE_WIRE_BIT          = 1 << 2,
  SCENE_HAIR_BIT          = 1 << 3,
  SCENE_PARTICLE_BIT      = 1 << 4,
  SCENE_TRANSPARENT_BIT   = 1 << 5,
  SCENE_DEBUG_BIT         = 1 << 6,

  PASS_DEFERRED           = SCENE_SKYBOX_BIT | SCENE_SOLID_BIT,
  PASS_FORWARD            = SCENE_EVERYTHING ^ PASS_DEFERRED,
};

// ----------------------------------------------------------------------------

// Interface for a scene run by the app.
class AppScene {
 public:
  AppScene() = default;
  virtual ~AppScene() {}

  virtual void init(Camera const& camera, views::Main &ui_mainview) = 0;
  virtual void deinit() = 0;

  virtual UIView* view() const = 0;

  virtual void update(float const dt, Camera &camera) = 0;
  virtual void render(Camera const& camera, unsigned int bitmask = SceneFilterBit::SCENE_EVERYTHING) = 0;
};

// ----------------------------------------------------------------------------

class App {
 public:
  struct Parameters_t {
    bool regulate_fps = true;
    bool postprocess  = true; //
    bool show_ui      = true;
  };

 public:
  App() :
    window_(nullptr),
    scene_(nullptr),
    deltatime_(0.0f),
    camera_(&arcball_controller_),
    ui_mainview_(nullptr)
  {}
  
  bool init(char const* title, AppScene *scene);
  void deinit();
  
  void run();
  
 private:
  void frame();
  void update_time();

  GLFWwindow *window_;
  AppScene *scene_;

  // Time.
  std::chrono::steady_clock::time_point time_;
  float deltatime_;

  // Camera.
  ArcBallController arcball_controller_;
  Camera camera_;

  // Gizmos handler.
  Gizmo gizmo_;

  // Postprocess.
  Postprocess postprocess_;

  // UI.
  UIController ui_controller_;
  views::Main *ui_mainview_;

  // UI accessible parameters.
  Parameters_t params_;

 private:
  App(App const&) = delete;
  App(App&&) = delete;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_APP_H_
