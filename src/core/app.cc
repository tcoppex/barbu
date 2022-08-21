#include "app.h"

#include "core/events.h"
#include "core/global_clock.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/views/views.h"

// ----------------------------------------------------------------------------

App::App()
  : window_{std::make_shared<Window>()} 
  , camera_{nullptr}
  , started_(false)
  , is_running_{true}
  , exit_status_{EXIT_SUCCESS}
  , rand_seed_{0u}
{}

App::~App() {
  if (started_) {
    ui_controller_.deinit();
    gx::Deinitialize();
  }

  // Resources.
  Assets::ReleaseAll();
  Resources::ReleaseAll();

  // Singletons.
  Events::Deinitialize();
  Logger::Deinitialize();
  GlobalClock::Deinitialize();
}

int32_t App::run(std::string_view title) {
  // Initialization.
  if (!presetup(title)) {
    LOG_ERROR( "The application fails to be initialized properly（￣□||||" );
    return EXIT_FAILURE;
  }
  
  // User's custom initialization.
  setup();

  // Check & update internal structures.
  postsetup();

  // Prepare a new frame for the mainloop, returns true when it continues.
  auto &events{ Events::Get() };
  auto const nextFrame{[this, &events]() {
    // Update internal data for the next frame.
    events.prepareNextFrame();

    // Check and dispatch events.
    return is_running_ && window_->poll(events);
  }};

  // Mainloop.
  while (nextFrame()) {
    // Update global clock and control the framerate.
    GlobalClock::Update(params_.regulate_fps);

    // Resources watchers for live-reload.
    Resources::WatchUpdate(Assets::UpdateAll);    
    
    // Update User interface.
    ui_controller_.update(window_); //

    // Frame magick.
    renderer_.frame(scene_, camera_,
      [this]() { update(); },
      [this]() { draw(); }
    );

    // Draw User interface..
    ui_controller_.render(params_.show_ui);

    // Swap front & back buffers.
    window_->flush();
  }

  // User's custom finalization.
  finalize();

  return exit_status_;
}

// ----------------------------------------------------------------------------

bool App::presetup(std::string_view title) {
  // Force stderr to flush automatically (ala GNU / Linux).
  std::setbuf(stderr, nullptr);

  // Initialize the standard C RNG seed, in case any third party use it.
  rand_seed_ = static_cast<uint32_t>(std::time(nullptr));
  std::srand(rand_seed_);

  // Singletons.
  GlobalClock::Initialize();
  Logger::Initialize();
  Events::Initialize(); //

  // Register the app for events callbacks dispatch.
  Events::Get().registerCallbacks(this);

  // Window and Graphics.
  {
    // Create the main window surface.
    if (started_ = window_->create(Display(), title); !started_) {
      LOG_ERROR( "The window creation failed (／。＼)" );
      return false;
    }

    // Initialize the Graphics API.
    gx::Initialize(window_);
  }

  // -------------------

  // Setup the Renderer.
  renderer_.init();

  // Setup the entity-components scene hierarchy.
  scene_.init();

  // -------------------

  // [improve] Setup the User Interface.
  {
    ui_controller_.init();
    ui_mainview_ = std::make_shared<views::Main>(params_);
    ui_controller_.set_mainview( ui_mainview_ );
    
    // Setup the renderer's UI with mainview.
    renderer_.params().sub_view = scene_.ui_view;
    if (auto ui = renderer_.ui_view; ui) {
      ui_mainview_->push_view( ui );
    }
    if (auto ui = renderer_.hair().ui_view; ui) {
      ui_mainview_->push_view( ui );
    }
    if (auto ui = renderer_.particle().ui_view; ui) {
      ui_mainview_->push_view( ui );
    }
  }

  return true;
}

void App::postsetup() {
  // Check camera settings.
  if (!camera_.initialized()) {
    LOG_WARNING( "The camera has not been initialized properly, a default one will be used instead." );
    if (window_) {
      camera_.setDefault(window_->resolution());
    } else {
      camera_.setDefault();
    }
  }
  if (!camera_.controller()) {
    LOG_WARNING( "The camera's view controller has not been set." );
  }
  camera_.rebuild();

  // Start the global clock post-initialization to avoid overhead.
  GlobalClock::Start();

  // Reindex imported objects.
  scene_.update(0.0f, camera_); //
}

// ----------------------------------------------------------------------------
