#include "app.h"

#include <thread>

#include "core/glfw.h"
#include "core/events.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/views/Main.h"

// ----------------------------------------------------------------------------

bool App::init(char const* title, AppScene *scene) {
  // System parameters.
  std::setbuf(stderr, nullptr);
  std::srand(static_cast<uint32_t>(std::time(nullptr)));

  // Init singletons.
  Logger::Initialize();

  if (!scene) {
    LOG_ERROR( "no scene was specified.");
    return false;
  }

  // Initialize Window Management API.
  if (!glfwInit()) {
    LOG_ERROR( "failed to initialize GLFW.");
    return false;
  }

  // Initialize OpenGL context flags.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, GLFW_DONT_CARE);

  // Compute window resolution from the main monitor's.
  float constexpr screenScale = 4.0f / 5.0f;
  GLFWvidmode const* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int const w = static_cast<int>(screenScale * mode->width);
  int const h = static_cast<int>(screenScale * mode->height);

  // Create the window and OpenGL context.
  window_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
  if (!window_) {
    LOG_ERROR( "failed to create the window.");
    glfwTerminate();
    return false;
  }

  // Make the window's context current.
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(0);
  
  // Init the event manager.
  InitEvents(window_); //

  // Initialize the Graphics API.
  gx::Initialize();

  // Preclean the screen.
  {
    glViewport(0, 0, w, h);
    glClearColor(0.25, 0.27, 0.23, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window_);
  }

  // -----------------

  // Camera setup.
  arcball_controller_.set_view(glm::pi<float>() / 16.0f, glm::pi<float>() / 8.0f, true);
  arcball_controller_.set_dolly(4.75f);
  camera_.set_perspective(glm::radians(60.0f), w, h, 0.01f, 500.0f);

  // Gizmo handler.
  gizmo_.init();

  // Postprocessing.
  postprocess_.init(camera_);

  // User Interface init.
  ui_controller_.init(window_);
  ui_mainview_ = new views::Main(params_);
  ui_controller_.set_mainview(ui_mainview_);

  // Initialize the scene.
  scene_ = scene;
  scene_->init(camera_, *ui_mainview_);

  // Start the chrono.
  time_ = std::chrono::steady_clock::now();

  return true;
}

void App::deinit() {
  postprocess_.deinit();
  gizmo_.deinit();
  scene_->deinit();

  ui_controller_.deinit();
  delete ui_mainview_;

  glfwTerminate();
  window_ = nullptr;

  Assets::ReleaseAll();
  Resources::ReleaseAll();
  gx::Deinitialize();

  Logger::Deinitialize();
}

void App::run() {
  while (!glfwWindowShouldClose(window_)) {
    // Manage events.
    HandleEvents();

    // Update and render one frame.
    frame();

    // Swap front & back buffers.
    glfwSwapBuffers(window_);
  }
}

// ----------------------------------------------------------------------------

void App::frame() {
  // Update UI.
  ui_controller_.update();

  // Resources watchers for live update.
  update_resources();

  // Generic updates.
  update_time();
  camera_.update(deltatime_);

  if ('h' == GetEventData().lastChar) {
    params_.show_ui ^= true;
  }

  //------------------------------------------  
  // Update scene.
  scene_->update(deltatime_, camera_);
  postprocess_.toggle(params_.postprocess); //

  // Camera specific rendering.
  gizmo_.begin_frame(deltatime_, camera_);
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // 'Deffered'-pass, post-process the solid objects.
    postprocess_.begin();
    scene_->render(camera_, SceneFilterBit::PASS_DEFERRED);
    postprocess_.end(camera_);

    // Forward-pass, render the special effects.
    scene_->render(camera_, SceneFilterBit::PASS_FORWARD);

    // (todo : have a final composition pass here).
  }
  gizmo_.end_frame(camera_);
  //------------------------------------------
  
  // Render UI.
  ui_controller_.render(params_.show_ui);
}

void App::update_resources() {
  // [ to put inside a thread, eventually ]

  // It would be more interesting to have different watch time depending on the resources,
  // (eg. a shader should be quicker to reload than a texture).
  float constexpr kLimitSeconds = 1.25f;
  static float current_tick = 0.0f; //
  
  if (current_tick > kLimitSeconds) {
    // release internal memory from last frames.
    Resources::ReleaseAll(); // 

    // watch for resource modifications.
    Resources::UpdateAll(); 

    // upload newly modified dependencies to their assets.
    Assets::UpdateAll();
    
    current_tick = 0.0f;
  }
  current_tick += deltatime_;
}

void App::update_time() {
  auto constexpr kMaxFPS = 90.0;
  auto constexpr fps_time = std::chrono::duration<double>(1.0 / (1.015 * kMaxFPS));

  auto local_update_time = [this]() {
    auto const tick      = std::chrono::steady_clock::now();
    auto const time_span = std::chrono::duration_cast<std::chrono::duration<double>>(tick - time_);
    time_      = tick;
    deltatime_ = static_cast<float>(time_span.count());
    return time_span;
  };

  auto const time_span = local_update_time();

  // Regulate FPS.
  if (params_.regulate_fps && (time_span < fps_time)) {
    std::this_thread::sleep_for(fps_time - time_span);
    local_update_time();
  }
}


// ----------------------------------------------------------------------------
