#ifndef BARBU_ECS_COMPONENTS_SKIN_H_
#define BARBU_ECS_COMPONENTS_SKIN_H_

#include <vector>

#include "ecs/component.h"

#include "fx/animation/common.h"
#include "fx/animation/skeleton.h"
#include "fx/animation/skeleton_controller.h"
#include "ecs/entity-fwd.h"

// ----------------------------------------------------------------------------

//
//  Apply skeleton animation for a mesh.
//
class SkinComponent final : public ComponentParams<Component::Skin> {
 public:
  using SkeletonMap_t = std::vector<EntityHandle>; //

 public:
  SkinComponent()
    : mode_{ SkinningMode::LinearBlending }
    , skeleton_{ nullptr }
  {}

  ~SkinComponent();

  bool update(float global_time);

  inline void setSkinningMode(SkinningMode const mode) noexcept {
    mode_ = mode;
  }

  inline void setSkeleton(SkeletonHandle skeleton) noexcept {
    skeleton_ = skeleton;
  }

  inline SkinningMode skinningMode() const noexcept {
    return mode_;
  }

  inline SkeletonHandle skeleton() const noexcept {
    return skeleton_;
  }

  inline SkeletonMap_t& skeletonMap() noexcept { 
    return skeleton_map_; 
  }

  inline Sequence_t& sequence() noexcept {
    return sequence_;
  }

  inline SkeletonController const& controller() const noexcept {
    return controller_;
  }

  inline uint32_t textureID() const noexcept { 
    return texture_id_; // 
  }

 private:
  /* Create the skinning data device buffer if needed and upload data to it. */
  void updateSkinningBuffer(); // [fixme]

  // Inputs.
  SkinningMode            mode_;
  SkeletonHandle          skeleton_;
  mutable SkeletonMap_t   skeleton_map_;  //
  mutable Sequence_t      sequence_;      //

  // Outputs.
  SkeletonController controller_;

  // [ fixme : raw texture buffer handles ]
  uint32_t buffer_id_  = 0u; //
  uint32_t texture_id_ = 0u; //
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_SKIN_H_