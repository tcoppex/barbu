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
    : camera_(nullptr)
    , window_(nullptr)
    , rand_seed_(0u)
    , deltatime_(0.0f)
    , ui_mainview_(nullptr)
  {}

  /* The destructor is the recommended place to clean the app when it finishes. */
  virtual ~App();
  
  /* Initialize & run the application mainloop. */

  /* Launch the application mainloop.
     Returns a non null value if the application fails, zero otherwise. */
  int32_t run(std::string_view title);

 protected:
  /* User defined, called once at initialization, after the constructor and
     before the loop. This is the recommended way to setup the App. */
  virtual void setup() {}

  /* User defined, call each frame before draw.
     This is the recommended place to update simulation states. */
  virtual void update() {}
  
  /* User defined, call each frame after update.
     This is the recommended place to declare custom rendering calls 
     which does not use the renderer directly. */
  virtual void draw() {}

  /* Returns the window resolution. */
  inline glm::ivec2 const& resolution() const {
    return resolution_;
  }

  /* Returns core app parameters. */
  inline Parameters_t& params() {
    return params_;
  }

  // Main camera accessible to the user, it's projection must be set before the mainloop start.
  Camera camera_;

  // Entity Component Scene hierarchy to fetch the renderer.
  SceneHierarchy scene_;

  // Pipeline defining how a scene is rendered.
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
  Parameters_t params_;
  std::shared_ptr<views::Main> ui_mainview_;

 private:
  App(App const&) = delete;
  App(App&&) = delete;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_APP_H_
