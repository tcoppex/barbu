#include "core/global_clock.h"

#include <chrono>
#include "core/logger.h"

// ----------------------------------------------------------------------------

Clock::Clock()
  : start_time_(0.0),
    delta_time_(0.0),
    frame_time_(0.0),
    last_fps_time_(0.0),
    application_time_(0.0),
    application_delta_time_(0.0),
    time_scale_(1.0),
    fps_(-1),
    current_second_framecount_(0u),
    framecount_(0u),
    default_unit_(TimeUnit::Second),
    bPaused_(true)
{
  converse_table_[TimeUnit::Nanosecond]   = 1.0e-9;
  converse_table_[TimeUnit::Microsecond]  = 1.0e-6;
  converse_table_[TimeUnit::Millisecond]  = 1.0e-3;
  converse_table_[TimeUnit::Second]       = 1.0;
  converse_table_[TimeUnit::Default]      = converse_table_[default_unit_];

  // initialize the clock
  start_time_ = absoluteTime();

  resume();
}

void Clock::update() {
  ++current_second_framecount_;
  ++framecount_;

  double lastFrameTime{frame_time_};
  frame_time_ = relativeTime(TimeUnit::Millisecond);
  delta_time_ = frame_time_ - lastFrameTime;

  if ((frame_time_ - last_fps_time_) >= 1000.0) {
    last_fps_time_ = frame_time_;
    fps_ = current_second_framecount_;
    current_second_framecount_ = 0u;
  }

  if (!isPaused()) {
    application_delta_time_ = time_scale_ * delta_time_;
    application_time_ += application_delta_time_;
  } else {
    application_delta_time_ = 0.0;
  }
}

void Clock::stabilizeDeltaTime(const double dt) {
  frame_time_ = relativeTime(TimeUnit::Millisecond) - dt;
  delta_time_ = dt;

  if (!isPaused()) {
    application_delta_time_ = time_scale_ * delta_time_;
    application_time_ += application_delta_time_;
  } else {
    application_delta_time_ = 0.0;
  }
}

bool Clock::isSameUnit(TimeUnit src, TimeUnit dst) const {
  return (src == dst) || (converse_table_[src] == converse_table_[dst]);
}

double Clock::convertTime(TimeUnit src_unit, TimeUnit dst_unit, const double time) const {
  double const scale{ 
    (isSameUnit(src_unit, dst_unit)) ? 1.0 : converse_table_[src_unit] / converse_table_[dst_unit]
  };
  return scale * time;
}

double Clock::absoluteTime(TimeUnit unit) const {
  // Note : 
  //  Using double cast difference instead of time span duration we would loose 
  //  precisions.
  auto const current_time{ std::chrono::system_clock::now() };
  auto const duration_in_seconds{ std::chrono::duration<double>(current_time.time_since_epoch()) };
  return convertTime(TimeUnit::Second, unit, duration_in_seconds.count());
}

double Clock::relativeTime(TimeUnit unit) const {
  return convertTime(TimeUnit::Millisecond, unit,
    absoluteTime(TimeUnit::Millisecond) - start_time_
  );
}

double Clock::deltaTime(TimeUnit unit) const {
  return convertTime(TimeUnit::Millisecond, unit, delta_time_);
}

double Clock::frameTime(TimeUnit unit) const {
  return convertTime(TimeUnit::Millisecond, unit, frame_time_);
}

double Clock::frameElapsedTime(TimeUnit unit) const {
  return relativeTime(unit) - frameTime(unit);
}

double Clock::applicationTime(TimeUnit unit) const {
  return convertTime(TimeUnit::Millisecond, unit, application_time_);
}

double Clock::applicationDeltaTime(TimeUnit unit) const {
  return convertTime(TimeUnit::Millisecond, unit, application_delta_time_);
}

void Clock::setDefaultUnit(TimeUnit unit) {
  default_unit_ = unit;
  converse_table_[TimeUnit::Default] = converse_table_[default_unit_];
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include <thread>

std::chrono::steady_clock::time_point GlobalClock::sTime;

void GlobalClock::Start() {
  // Resume the clock post-initialization to skip initialization overhead. 
  GlobalClock::Get().resume();

  // Start the FPS chrono (the framerate is regulated through a local timer).
  sTime = std::chrono::steady_clock::now();
}

void GlobalClock::Update(bool bRegulateFPS) {
  auto constexpr kMaxFPS{ 90.0 };
  auto constexpr kTargetFPSTime{ std::chrono::duration<double>(1.0 / (1.015 * kMaxFPS)) };

  // Regulate FPS.
  {
    auto local_update_time{[]() {
      auto const tick{ std::chrono::steady_clock::now() };
      auto const time_span{ std::chrono::duration_cast<std::chrono::duration<double>>(tick - sTime) };
      sTime = tick;
      return time_span;
    }};

    auto const time_span{ local_update_time() };
    if (bRegulateFPS && (time_span < kTargetFPSTime)) {
      std::this_thread::sleep_for(kTargetFPSTime - time_span);
      local_update_time();
    }
  }

  // Update the global clock.  
  {
    auto &gc{ Get() };

    // The first few clock updates can occurs a very long time after the app initialization.
    // Therefore we stabilize the dt the first few frames. [ improve ? ]
    if (gc.framecount() < 2) {
      gc.stabilizeDeltaTime( 1000.0 / kMaxFPS );
    }
    gc.update();

    // LOG_INFO( gc.deltaTime(), gc.applicationDeltaTime(), gc.frameElapsedTime(), gc.applicationTime()  );
    // LOG_INFO( gc.timeScale(), gc.fps(), gc.framecount() ); 
  }
}

// ----------------------------------------------------------------------------

