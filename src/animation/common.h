#ifndef BARBU_ANIMATION_COMMON_H_
#define BARBU_ANIMATION_COMMON_H_

#include <vector>
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/dual_quaternion.hpp"

// -----------------------------------------------------------------------------
// Common datastructures used for animations.
// -----------------------------------------------------------------------------

// Abstract structure for specific animation.
struct Action_t {
  virtual ~Action_t() {}
  virtual float duration() const = 0;

  char *name_ptr  = nullptr;                  //< pointer to the action name
  bool bLoop      = false;                    //< true if the action is looping
};

// -----------------------------------------------------------------------------

// Wrapper type used to store joint data.
template<typename T>
using JointBuffer_t = std::vector<T>;

// -----------------------------------------------------------------------------

// Single joint transformations.
struct JointPose_t {
  glm::quat qRotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  glm::vec3 vTranslation = glm::vec3(0.0f);
  float     fScale       = 1.0f;
};

// Set of joints transformation for a single frame. 
struct AnimationSample_t {
  JointBuffer_t<JointPose_t> joints;
};

// Set of AnimationSamples in a timeframe.
struct AnimationClip_t : Action_t {
  float duration() const override {
    return framecount / framerate;
  }

  std::vector<AnimationSample_t> samples;     //< buffer of samples
  uint32_t  framecount = 0u;                  //< total number of frames
  float     framerate  = 0.0f;                //< framerate in seconds
};


// -----------------------------------------------------------------------------

// TODO : Expression

// -----------------------------------------------------------------------------

// TODO : SequenceClip

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_COMMON_H_
