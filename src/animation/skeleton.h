#ifndef BARBU_ANIMATION_SKELETON_H_
#define BARBU_ANIMATION_SKELETON_H_

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "glm/glm.hpp"

#include "animation/common.h"

// -----------------------------------------------------------------------------

//
//  Hold the internal attributes of a skeleton rig used for skinning animation.
//
//  Notes :
//    * names should probably be pointer to an external string buffer.
//
//    * likewise index_map could be optimized to not store strings (or be temporary).
//
//    * global_bind_matrices are used only for debug display and can be discarded for
//      production.
//
struct Skeleton {
  // Joints data.
  JointBuffer_t<std::string>    names;
  JointBuffer_t<int32_t>        parents;
  JointBuffer_t<glm::mat4>      inverse_bind_matrices;
  JointBuffer_t<glm::mat4>      global_bind_matrices;

  // LookUp for joint index.
  std::unordered_map<std::string, int32_t> index_map;

  // Animation clips.
  std::vector<AnimationClip_t>  clips;

  Skeleton() = default;

  Skeleton(size_t _njoints) {
    names.reserve(_njoints);
    parents.reserve(_njoints);
    inverse_bind_matrices.reserve(_njoints);
  }

  inline int32_t njoints() const {
    LOG_CHECK( !inverse_bind_matrices.empty() );
    return static_cast<int32_t>(inverse_bind_matrices.size());
  }
  
  inline int32_t nclips() const {
    return static_cast<int32_t>(clips.size());
  }

  void add_joint(std::string_view name, int32_t parent_id, glm::mat4 const& inverse_bind_matrix);

  // Apply a matrix to the inverse_bind_matrices buffer.
  void transform_inverse_bind_matrices(glm::mat4 const& inv_world);

  // Fill the buffer of globals inverse bind matrices from local ones. 
  // (inv_locals could be a reference to the inverse global bind buffer)
  void calculate_globals_inverse_bind_from_locals(JointBuffer_t<glm::mat4> const& inv_locals, glm::mat4 const& inv_world = glm::mat4(1.0f));

  // Fill the global bind matrices (used to debug display rest joints).
  void calculate_global_bind_matrices();
};

using SkeletonHandle = std::shared_ptr<Skeleton>;

// -----------------------------------------------------------------------------

#endif  // BARBU_ANIMATION_SKELETON_H_
