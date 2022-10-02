#ifndef BARBU_CORE_GLOBAL_CLOCK_H_
#define BARBU_CORE_GLOBAL_CLOCK_H_

// ----------------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <array>

#include "memory/enum_array.h"
#include "utils/singleton.h"

// ----------------------------------------------------------------------------

// Time unit used by the clock.
enum class TimeUnit {
  Nanosecond,
  Microsecond,
  Millisecond,
  Second,
  Default,
  
  kCount,
};

// ----------------------------------------------------------------------------

//
// The global clock is used across an application to measure time.
// Time is returned in seconds by default.
//
class Clock {
 public:
  Clock();

  // pause the application timeline.
  inline void pause() noexcept { bPaused_ = true; }

  // resume the application timeline.
  inline void resume() noexcept { bPaused_ = false; }

  // Update per-frame time values.
  void update();

  // Convert time from one unit to another.
  double convertTime(TimeUnit src_unit, TimeUnit dst_unit, double const time) const;

  // -------------------

  // Time from the system.
  double absoluteTime(TimeUnit unit = TimeUnit::Default) const;
  
  // Time from the start of the clock.
  double relativeTime(TimeUnit unit = TimeUnit::Default) const;
  
  // Relative time at the beginning of the frame.
  double frameTime(TimeUnit unit = TimeUnit::Default) const;

  // Time from the last frame, does not stop with pause.
  double deltaTime(TimeUnit unit = TimeUnit::Default) const;

  // -------------------

  // Time of the application.
  double applicationTime(TimeUnit unit = TimeUnit::Default) const;

  // Delta time of the application (ie. 0.0 when paused).
  double applicationDeltaTime(TimeUnit unit = TimeUnit::Default) const;

  // Time from the beginning of the frame (relative_time - frame_time)
  double frameElapsedTime(TimeUnit unit = TimeUnit::Default) const;

  // -------------------

  inline double   timeScale()    const noexcept { return time_scale_; }
  inline int32_t  fps()           const noexcept { return fps_; }
  inline int32_t  framecount()    const noexcept { return framecount_; }
  inline bool     isPaused()     const noexcept { return bPaused_; }

  // Defines a scale by which application time is updated.
  void setTimeScale(double scale);

  // Specify the default time unit to return.
  void setDefaultUnit(TimeUnit unit);

  // Fixed first frames deltas.
  void stabilizeDeltaTime(double const dt);

 private:
  bool isSameUnit(TimeUnit src, TimeUnit dst) const;

  EnumArray<double, TimeUnit> converse_table_;

  double start_time_;                     //  global time at the beginning of the clock
  double delta_time_;                     //  duration of last frame
  double frame_time_;                     //  relative time at the beginning of the frame
  double last_fps_time_;                  //  last frame relative time (to count fps)

  double application_time_;               //  timeline of the application
  double application_delta_time_;         //  deltatime of the application
  double time_scale_;                     //  application time scale

  int32_t fps_;                           //  last second frame count
  int32_t current_second_framecount_;     //  current second frame count
  int32_t framecount_;                    //  total frame elapsed from the start

  TimeUnit default_unit_;                 //  default unit used to retrieve time

  bool bPaused_;                          //  when true, do not update application time
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include <chrono>

// [ use singleton of a specialized "AppClock" instead, with custom update ]
class GlobalClock final : public Singleton<Clock> {
 public:
  static void Start();
  static void Update(bool bRegulateFPS = true);

 private:
  // Used to regulate the framerate.
  static std::chrono::steady_clock::time_point sTime;

 private:
  friend class Singleton<Clock>;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_GLOBAL_CLOCK_H_
