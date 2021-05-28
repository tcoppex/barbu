#include "animation/common.h"

bool SequenceClip_t::evaluate_localtime(float const global_time, float &local_time) const {
  assert( nullptr != action_ptr );

  float const clip_duration = action_ptr->duration();
  LOG_CHECK( clip_duration > glm::epsilon<float>() );

  local_time = global_time - global_start;
  local_time *= abs(rate);

  // clamp the local time if the action loops a finite number of time.
  if (!action_ptr->bLoop || (action_ptr->bLoop && (nloops > 0))) {
    int32_t const total_loops = (action_ptr->bLoop) ? nloops : 1u;
    float const finish_time = total_loops * clip_duration;

    if (local_time >= finish_time) {
      return true;
    }
    local_time = glm::clamp(local_time, 0.0f, finish_time);
  }
  
  // loop the action.
  int32_t loop_id = 0;
  if (action_ptr->bLoop) {
    loop_id    = static_cast<int32_t>(local_time / clip_duration);
    local_time = fmodf( local_time, clip_duration);
  }

  // Handles reverse playback with ping-pong.
  bool const bReversed_a = (rate < 0.0f);
  bool const bReversed_b = bPingPong && ((loop_id & 1) == bReversed_a);
  bool const bReversed   = bReversed_a != bReversed_b;
  if (bReversed) {
    local_time = clip_duration - local_time;
  }

  if constexpr(true) {
    // Smooth-in / Smooth-out ping-pong loops.
    if (bPingPong) {
      float clip_phase = local_time / clip_duration;
            clip_phase = glm::smoothstep(0.0f, 1.0f, clip_phase);
      local_time = clip_phase * clip_duration;
    }
  }

  return false;
}
