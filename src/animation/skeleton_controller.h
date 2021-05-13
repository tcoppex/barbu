#ifndef BARBU_ANIMATION_SKELETON_CONTROLLER_H_
#define BARBU_ANIMATION_SKELETON_CONTROLLER_H_

#include <cassert>
#include "animation/common.h"
#include "animation/skeleton.h"

// -----------------------------------------------------------------------------

// 
// The SkeletonController transform a sequence of animation clips into skinning 
// data for different stages. 
//
class SkeletonController {
 public:
  SkeletonController() = default;

  JointBuffer_t<glm::mat4> const& global_pose_matrices() const {
    return global_pose_matrices_;
  }

  JointBuffer_t<glm::mat3x4> const& skinning_matrices() const {
    assert(!skinning_matrices_.empty());
    return skinning_matrices_;
  }

  JointBuffer_t<glm::dualquat> const& dual_quaternions() const {
    assert(!dual_quaternions_.empty());
    return dual_quaternions_;
  }

  /* Fill the in-between skinning matrices for the given time and sequence. 
   * Return true when it succeeds, false when no data where computed. */
  bool evaluate(SkinningMode const mode, SkeletonHandle skeleton, float const global_time, Sequence_t &sequence);

 private:
  /* Apply a normalized blending on previously sampled clips
   * and store the final local pose. */
  void blend_poses(Sequence_t const& sequence);

  /* Generate global pose matrices (eg. for post-processing). */
  void generate_global_pose_matrices(SkeletonHandle skeleton);

  /* Generate final transformation datas for skinning. */
  void generate_skinning_datas(SkinningMode const mode, SkeletonHandle skeleton);

  int32_t njoints_;
  AnimationSample_t             local_pose_;
  JointBuffer_t<glm::mat4>      global_pose_matrices_;
  JointBuffer_t<glm::mat3x4>    skinning_matrices_;
  JointBuffer_t<glm::dualquat>  dual_quaternions_;
};

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_SKELETON_CONTROLLER_H_
