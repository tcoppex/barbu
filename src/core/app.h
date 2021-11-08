#ifndef BARBU_CORE_APP_H_
#define BARBU_CORE_APP_H_

// ----------------------------------------------------------------------------

#include <cstdlib>
#include <string_view>

#include "glm/vec2.hpp"

#include "core/window.h"
#include "core/camera.h"
#include "core/renderer.h"
#include "ecs/scene_hierarchy.h"
#include "ui/ui_controller.h" //

namespace views {
  class Main;
}

// ----------------------------------------------------------------------------

class App : public EventCallbacks {
 public:
  struct Parameters_t {
    bool regulate_fps = true;
    bool show_ui      = true;
  };

 public:
  App();

  /* The destructor is the recommended place to clean the app when it finishes. */
  virtual ~App();

  /* Launch the application mainloop.
     Returns a non null value if the application fails, zero otherwise. */
  int32_t run(std::string_view title);

  /* Flag the application to exit. */
  inline void quit(int32_t status = EXIT_SUCCESS) noexcept {
    is_running_ = false;
    exit_status_ = status;
  }

 protected:
  /* [User defined] Called once at initialization after the constructor and
     before the loop. This is the recommended way to setup the App. */
  virtual void setup() {}

  /* [User defined] Called each frame before 'draw'.
     This is the recommended place to update simulation states. */
  virtual void update() {}
  
  /* [User defined] Called each frame after 'update'.
     This is the recommended place to declare custom rendering calls 
     which does not use the renderer directly. */
  virtual void draw() {}

  /* [User defined] Optional place to call termination code before exiting.
   * Prefer the destructor. */
  virtual void finalize() {}

  // -------

  /* Returns core app parameters. */
  inline Parameters_t& params() noexcept {
    return params_;
  }

  /* Display / Hide UI */
  inline void toggleUI() noexcept {
    params_.show_ui ^= true;
  }

  /* [temporary] Return the resolution of the current window. */
  inline glm::ivec2 resolution() const {
    return glm::ivec2( window_->width(), window_->height()); //
  }

  /* [temporary] */
  Renderer::Parameters_t& getRendererParameters() {
    return renderer_.params();
  }

  // -------

 private:
  /* Initialize the core app. */
  bool presetup(std::string_view title);

  /* Post user initialization */
  void postsetup();

 public:
  std::shared_ptr<views::Main> ui_mainview_;    //<! [PUBLIC] UI View.

 protected:
  WindowHandle window_;                         //<! Interface to the main window.
  Renderer renderer_;                           //<! Pipeline defining how a scene is rendered.
  Camera camera_;                               //<! Main camera accessible to the user.
  SceneHierarchy scene_;                        //<! Entity Component Scene hierarchy to fetch the renderer.

 private:
  bool started_;
  bool is_running_;                             //<! True while the mainloop continue.
  int32_t exit_status_;                         //<! Status used when exiting the process.
  uint32_t rand_seed_;                          //<! Random Number Generator seed.
  UIController ui_controller_;                  //<! Manage the UI application-wise.
  Parameters_t params_; //                      //<! Generics parameters modifiable by UI.

 private:
  App(App const&) = delete;
  App(App&&) = delete;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_APP_H_
