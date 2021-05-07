#ifndef BARBU_UTILS_GLOBAL_CLOCK_H_
#define BARBU_UTILS_GLOBAL_CLOCK_H_

#include <cstdint>
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

//
// The global clock is used across an application to measure time.
// Time is returned in seconds by default.
//
class Clock {
 public:
  Clock();

  // pause the application timeline.
  void pause() { bPaused_ = true; }

  // resume the application timeline.
  void resume() { bPaused_ = false; }

  // Update per-frame time values.
  void update();

  // Convert time from one unit to another.
  double convert_time(TimeUnit src_unit, TimeUnit dst_unit, double const time) const;

  // -------------------

  // Time from the system.
  double    absolute_time(TimeUnit unit = TimeUnit::Default) const;
  
  // Time from the start of the clock.
  double    relative_time(TimeUnit unit = TimeUnit::Default) const;
  
  // Relative time at the beginning of the frame.
  double       frame_time(TimeUnit unit = TimeUnit::Default) const;

  // Time from the last frame (does not stop with pause).
  double       delta_time(TimeUnit unit = TimeUnit::Default) const;

  // -------------------

  // Time of the application.
  double application_time(TimeUnit unit = TimeUnit::Default) const;

  // Delta time of the application (ie. stop 0 when paused).
  double application_delta_time(TimeUnit unit = TimeUnit::Default) const;

  // Time from the beginning of the frame (relative_time - frame_time)
  double frame_elapsed_time(TimeUnit unit = TimeUnit::Default) const;

  // -------------------

  double        time_scale() const { return time_scale_; }
  int32_t              fps() const { return fps_; }
  int32_t framecount_total() const { return framecount_total_; }
  bool           is_paused() const { return bPaused_; }

  // Defines a scale by which application time is updated.
  void set_time_scale(double scale);

  // Specify the default time unit to return.
  void set_default_unit(TimeUnit unit);

  // Fixed first frames deltas.
  void stabilize_delta_time(double const dt);

 private:
  bool is_same_unit(TimeUnit src, TimeUnit dst) const;

  EnumArray<double, TimeUnit> converse_table_;

  double start_time_;              //  global time at the beginning of the clock
  double delta_time_;              //  duration of last frame
  double frame_time_;              //  relative time at the beginning of the frame
  double last_fps_time_;           //  last frame relative time (to count fps)

  double application_time_;        //  timeline of the application
  double application_delta_time_;  //  deltatime of the application
  double time_scale_;              //  application time scale

  int32_t fps_;                    //  last second frame count
  int32_t framecount_;             //  current second frame count
  int32_t framecount_total_;       //  total frame elapsed from the start

  TimeUnit default_unit_;          //  default unit used to retrieve time

  bool bPaused_;                   //  when true, do not update application time
};

class GlobalClock : public Singleton<Clock> {
  friend class Singleton<Clock>;
};

// ----------------------------------------------------------------------------

#endif  // BARBU_UTILS_GLOBAL_CLOCK_H_
