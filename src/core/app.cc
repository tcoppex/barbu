#include "app.h"

#include <cstdlib>
#include <thread>

#include "core/glfw.h"
#include "core/events.h"
#include "core/global_clock.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

App::~App() {
  if (!window_) {
    return;
  }

  ui_controller_.deinit();

  glfwTerminate();
  window_ = nullptr;

  Assets::ReleaseAll();
  Resources::ReleaseAll();
  gx::Deinitialize();

  Logger::Deinitialize();
  GlobalClock::Deinitialize();
}

int32_t App::run(std::string_view title) {
  // Pre-Initialization.
  if (!presetup(title)) {
    return EXIT_FAILURE;
  }

  // Mainloop.
  while (!glfwWindowShouldClose(window_)) {
    // Events.
    HandleEvents();

    // User interface.
    ui_controller_.update();

    // Clock (with framerate control).
    update_time();

    // Resources watchers (for live reload).
    Resources::WatchUpdate(deltatime_, Assets::UpdateAll);

    // ------------------------------------------
    // Frame magick.
    renderer_.frame( scene_, camera_, 
      // Update.
      [this]() {
        update();
        camera_.update(deltatime_); //
        scene_.update(deltatime_, camera_); //
      },

      // Render
      [this]() {
        draw();
      }
    );
    // ------------------------------------------

    // Render User Interface.
    ui_controller_.render(params_.show_ui);

    // Swap front & back buffers.
    glfwSwapBuffers(window_);
  }

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------

bool App::presetup(std::string_view title) {
  // System parameters.
  std::setbuf(stderr, nullptr);
  rand_seed_ = static_cast<uint32_t>(std::time(nullptr));
  std::srand(rand_seed_); //

  // Initialize singletons.
  GlobalClock::Initialize();
  Logger::Initialize();

  // Initialize Window Management API.
  if (!glfwInit()) {
    LOG_ERROR( "GLFW failed to be initialized." );
    return false;
  }

  // Initialize OpenGL context flags.
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR,  4);
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR,  6);
  glfwWindowHint( GLFW_OPENGL_PROFILE,         GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT,  GLFW_TRUE);
  glfwWindowHint( GLFW_DOUBLEBUFFER,           GLFW_TRUE);
  glfwWindowHint( GLFW_SAMPLES,                GLFW_DONT_CARE);
  glfwWindowHint( GLFW_RESIZABLE,              GLFW_FALSE);

  // Compute window resolution from the main monitor's.
  float constexpr kScreenScale = 0.82f;
  GLFWvidmode const* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  resolution_.x = glm::trunc(kScreenScale * mode->width);
  resolution_.y = glm::trunc(kScreenScale * mode->height);

  // Create the window and OpenGL context.
  window_ = glfwCreateWindow( resolution_.x, resolution_.y, title.data(), nullptr, nullptr);
  if (!window_) {
    LOG_ERROR( "The window creation failed." );
    glfwTerminate();
    return false;
  }

  // Make the window's context current.
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(0);
  
  // Init the event manager.
  InitEvents(window_);

  // Initialize the Graphics API.
  gx::Initialize();

  // Preclean the screen.
  {
    gx::Viewport( resolution_.x, resolution_.y);
    gx::ClearColor(0.75f, 0.27f, 0.23f);
    glClear(GL_COLOR_BUFFER_BIT); //
    glfwSwapBuffers(window_);
  }

  // User Interface init.
  ui_controller_.init(window_);
  ui_mainview_ = std::make_shared<views::Main>(params_);
  ui_controller_.set_mainview( ui_mainview_ );
 
  // Entity-Component Scene handler.
  scene_.init();

  // Renderer.
  renderer_.init();
  
  // Setup renderer UI with mainview [improve].
  {
    renderer_.params().sub_view = scene_.ui_view;
    if (auto ui = renderer_.ui_view; ui) {
      ui_mainview_->push_view( ui );
    }
    if (auto ui = renderer_.hair().ui_view; ui) {
      ui_mainview_->push_view( ui );
    }
    if (auto ui = renderer_.particle().ui_view; ui) {
      //ui_mainview_->push_view( ui );
    }
  }

  // User initialization.
  setup();

  // ---------------------

  // Check camera settings.
  if (!camera_.initialized()) {
    LOG_ERROR( "The camera projection has not been properly initialized (￣ε￣)" );
    camera_.set_default();
  }
  camera_.rebuild();

  // Resume clock.
  {
    // Resume the clock post-initialization to skip initialization overhead. 
    GlobalClock::Get().resume();

    // Start the FPS chrono (the framerate is regulated through a local timer).
    time_ = std::chrono::steady_clock::now();
  }

  // Reindex import objects.
  scene_.update(deltatime_, camera_); //

  return true;
}

// ----------------------------------------------------------------------------

void App::update_time() {
  auto constexpr kMaxFPS = 90.0;
  auto constexpr fps_time = std::chrono::duration<double>(1.0 / (1.015 * kMaxFPS));

  // Regulate FPS.
  {
    auto local_update_time = [this]() {
      auto const tick      = std::chrono::steady_clock::now();
      auto const time_span = std::chrono::duration_cast<std::chrono::duration<double>>(tick - time_);
      time_ = tick;
      //deltatime_ = static_cast<float>(time_span.count()); // [overwritten]
      return time_span;
    };

    auto const time_span = local_update_time();
    if (params_.regulate_fps && (time_span < fps_time)) {
      std::this_thread::sleep_for(fps_time - time_span);
      local_update_time();
    }
  }

  // Update the global clock.  
  {
    auto &gc = GlobalClock::Get();

    // The first few clock updates can occurs a very long time after the app initialization.
    // Therefore we stabilize the dt the first few frames. [ improve ? ]
    if (gc.framecount_total() < 2) {
      gc.stabilize_delta_time( 1000.0 / kMaxFPS );
    }
    gc.update();

    // Bypass previous "precise" deltatime using global clock.
    deltatime_ = gc.delta_time();
    
    // LOG_INFO( gc.delta_time(), gc.application_delta_time(), gc.frame_elapsed_time(), gc.application_time()  );
    // LOG_INFO( gc.time_scale(), gc.fps(), gc.framecount_total() ); 
  }
}

// ----------------------------------------------------------------------------
