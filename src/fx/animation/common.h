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
  bool evaluate_localtime(float const global_time, float &local_time) const;

  // Return the phase of the animation given the local_time.
  float phase(float const local_time) const {
    assert( nullptr != action_ptr );
    return local_time / action_ptr->duration();
  }
};

using Sequence_t = std::vector<SequenceClip_t>;

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_COMMON_H_
