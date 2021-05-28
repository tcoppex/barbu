#ifndef BARBU_ANIMATION_COMMON_H_
#define BARBU_ANIMATION_COMMON_H_

#include <string>
#include <vector>
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/dual_quaternion.hpp"

#include "core/logger.h"

// -----------------------------------------------------------------------------

// Differents mode of skin vertex interpolation.
enum class SkinningMode {
  LinearBlending,
  DualQuaternion,
  kCount
};

// -----------------------------------------------------------------------------

// Wrapper type used to store joint data.
template<typename T>
using JointBuffer_t = std::vector<T>;

// Single joint transformations.
struct JointPose_t {
  glm::quat qRotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  glm::vec3 vTranslation = glm::vec3(0.0f);
  float     fScale       = 1.0f; // [not used]
};

// Set of joints transformation for a single frame. 
struct AnimationSample_t {
  JointBuffer_t<JointPose_t> joints;
};

using AnimationSampleBuffer_t = std::vector<AnimationSample_t>;

// -----------------------------------------------------------------------------

// Abstract structure for specific animation.
struct Action_t {
  Action_t() = default;
  Action_t(std::string_view _name)
    : name(_name)
  {}
  virtual ~Action_t() {}
  virtual float duration() const = 0;

  std::string name;                           //< [ pointer to the action name ]
  bool bLoop = false;                         //< true if the action is looping
};

// Set of skinning animation in a timeframe.
struct AnimationClip_t : Action_t {
  AnimationSampleBuffer_t samples;            //< buffer of samples
  int32_t framecount = 0;                     //< total number of frames
  float framerate = 0.0f;                     //< framerate in seconds
  
  AnimationClip_t() = default;

  AnimationClip_t(std::string_view _name, int32_t _size, float _duration) 
    : Action_t(_name)
    , framecount(_size)
    , framerate(framecount / _duration) //
  {
    LOG_DEBUG_INFO("* AnimationClip :", _name, _size, _duration, framerate);
    samples.resize(_size);
  }

  float duration() const override {
    return framecount / framerate;
  }
};

// TODO : Expression.

// -----------------------------------------------------------------------------

// Animation being played.
struct SequenceClip_t {
  Action_t* action_ptr = nullptr; // [ bug prone as hell ]

  float global_start   = 0.0f;
  float rate           = 1.0f;
  float weight         = 1.0f;

  int32_t nloops       = 0;
  bool bEnable         = false;
  bool bPingPong       = false;

  explicit 
  SequenceClip_t(Action_t* _action_ptr = nullptr)
    : action_ptr(_action_ptr)
    , bEnable(true)
  {}

  // Compute the localtime for the sequence, looping the animation if needed.
  // @return true when the sequence has ended.
  bool evaluate_localtime(float const global_time, float &local_time) const {
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

  // Return the phase of the animation given the local_time.
  float phase(float const local_time) const {
    assert( nullptr != action_ptr );
    return local_time / action_ptr->duration();
  }
};

using Sequence_t = std::vector<SequenceClip_t>;

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_COMMON_H_
