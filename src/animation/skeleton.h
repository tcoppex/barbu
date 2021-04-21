#ifndef BARBU_ANIMATION_SKELETON_H_
#define BARBU_ANIMATION_SKELETON_H_

#include <vector>
#include <string>
#include <memory>

#include "glm/glm.hpp"

#include "animation/common.h"

// -----------------------------------------------------------------------------

struct Skeleton {
  // Joints data.
  JointBuffer_t<std::string>  names; //
  JointBuffer_t<int32_t>      parents;
  JointBuffer_t<glm::mat4>    inverse_bind_matrices;

  // [debug]
  JointBuffer_t<glm::mat4>    global_bind_matrices;

  // Animation clips.
  std::vector<AnimationClip_t> clips;

  Skeleton() = default;

  Skeleton(size_t _njoints) {
    names.reserve(_njoints);
    parents.reserve(_njoints);
    inverse_bind_matrices.reserve(_njoints);
    global_bind_matrices.reserve(_njoints);
  }

  inline int32_t njoints() const {
    return static_cast<int32_t>(inverse_bind_matrices.size());
  }
  
  inline int32_t nclips() const {
    return static_cast<int32_t>(clips.size());
  }

  inline void add_joint(std::string_view name, int32_t parent_id, glm::mat4 const& inverse_bind_matrix) {
    names.push_back(name.data());
    parents.push_back(parent_id);
    inverse_bind_matrices.push_back(inverse_bind_matrix);
  }
};

using SkeletonHandle = std::shared_ptr<Skeleton>;

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_SKELETON_H_
