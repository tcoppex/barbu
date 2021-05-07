#include "core/global_clock.h"
#include <chrono>

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
    framecount_(0u),
    framecount_total_(0u),
    default_unit_(TimeUnit::Second),
    bPaused_(true)
{
  converse_table_[TimeUnit::Nanosecond]   = 1.0e-9;
  converse_table_[TimeUnit::Microsecond]  = 1.0e-6;
  converse_table_[TimeUnit::Millisecond]  = 1.0e-3;
  converse_table_[TimeUnit::Second]       = 1.0;
  converse_table_[TimeUnit::Default]      = converse_table_[default_unit_];

  // initialize the clock
  start_time_ = absolute_time();

  resume();
}

void Clock::update() {
  ++framecount_;
  ++framecount_total_;

  double lastFrameTime = frame_time_;
  frame_time_ = relative_time(TimeUnit::Millisecond);
  delta_time_ = frame_time_ - lastFrameTime;

  if ((frame_time_ - last_fps_time_) >= 1000.0) {
    last_fps_time_ = frame_time_;
    fps_ = framecount_;
    framecount_ = 0u;
  }

  if (!is_paused()) {
    application_delta_time_ = time_scale_ * delta_time_;
    application_time_ += application_delta_time_;
  } else {
    application_delta_time_ = 0.0;
  }
}

void Clock::stabilize_delta_time(const double dt) {
  frame_time_ = relative_time(TimeUnit::Millisecond) - dt;
  delta_time_ = dt;

  if (!is_paused()) {
    application_delta_time_ = time_scale_ * delta_time_;
    application_time_ += application_delta_time_;
  } else {
    application_delta_time_ = 0.0;
  }
}

bool Clock::is_same_unit(TimeUnit src, TimeUnit dst) const {
  return (src == dst) || (converse_table_[src] == converse_table_[dst]);
}

double Clock::convert_time(TimeUnit src_unit, TimeUnit dst_unit, const double time) const {
  const double scale = (is_same_unit(src_unit, dst_unit)) ? 1.0 : 
      converse_table_[src_unit] / converse_table_[dst_unit]
  ;
  return scale * time;
}

double Clock::absolute_time(TimeUnit unit) const {
  double global_time = 0.0;

  // Note : 
  //  Using double cast difference instead of time span duration we would loose 
  //  precisions.

  auto const current_time = std::chrono::system_clock::now();
  auto const duration_in_seconds = std::chrono::duration<double>(current_time.time_since_epoch());
  global_time = duration_in_seconds.count();
  global_time = convert_time(TimeUnit::Second, unit, global_time);

  return global_time;
}

double Clock::relative_time(TimeUnit unit) const {
  double t = absolute_time(TimeUnit::Millisecond) - start_time_;
  return convert_time(TimeUnit::Millisecond, unit, t);
}

double Clock::delta_time(TimeUnit unit) const {
  return convert_time(TimeUnit::Millisecond, unit, delta_time_);
}

double Clock::frame_time(TimeUnit unit) const {
  return convert_time(TimeUnit::Millisecond, unit, frame_time_);
}

double Clock::frame_elapsed_time(TimeUnit unit) const {
  return relative_time(unit) - frame_time(unit);
}

double Clock::application_time(TimeUnit unit) const {
  return convert_time(TimeUnit::Millisecond, unit, application_time_);
}

double Clock::application_delta_time(TimeUnit unit) const {
  return convert_time(TimeUnit::Millisecond, unit, application_delta_time_);
}

void Clock::set_default_unit(TimeUnit unit) {
  default_unit_ = unit;
  converse_table_[TimeUnit::Default] = converse_table_[default_unit_];
}

// ----------------------------------------------------------------------------

