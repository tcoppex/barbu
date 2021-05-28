#ifndef BARBU_CORE_APP_H_
#define BARBU_CORE_APP_H_

// ----------------------------------------------------------------------------

#include <chrono>
#include <string_view>
#include "glm/vec2.hpp"

#include "core/camera.h"
#include "core/renderer.h"
#include "ecs/scene_hierarchy.h"
#include "ui/ui_controller.h"

struct GLFWwindow;
namespace views {
  class Main;
}

// ----------------------------------------------------------------------------

class App {
 public:
  struct Parameters_t {
    bool regulate_fps = true;
    bool show_ui      = true;
  };

  App()
    : ui_mainview_(nullptr)
    , camera_(nullptr)
    , window_(nullptr)
    , rand_seed_(0u)
    , deltatime_(0.0f)
  {}

  virtual ~App();
  
  /* Initialize then run the application mainloop. */
  int32_t run(std::string_view title);

  glm::ivec2 const& resolution() const {
    return resolution_;
  }

  // User interface data are public.
  std::shared_ptr<views::Main> ui_mainview_;
  Parameters_t params_; //

 protected:
  virtual void setup() {}
  virtual void update() {}
  virtual void draw() {}

  Camera camera_;
  SceneHierarchy scene_;
  Renderer renderer_;

 private:
  /* Initialize the core app. */
  bool presetup(std::string_view title);

  /* Update the global clock & regulate framecontrol. */
  void update_time();

  // Window.
  GLFWwindow *window_;
  glm::ivec2 resolution_;

  // RNG seed.
  uint32_t rand_seed_;

  // Time.
  std::chrono::steady_clock::time_point time_;
  float deltatime_; 

  // User Interface.
  UIController ui_controller_;

 private:
  App(App const&) = delete;
  App(App&&) = delete;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_APP_H_
