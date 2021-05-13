#ifndef BARBU_ECS_COMPONENTS_SKIN_H_
#define BARBU_ECS_COMPONENTS_SKIN_H_

#include "ecs/component.h"

#include "animation/common.h"
#include "animation/skeleton.h"
#include "animation/skeleton_controller.h"

// ----------------------------------------------------------------------------

//
//  Apply skeleton animation for a mesh.
//
class SkinComponent final : public ComponentParams<Component::Skin> {
 public:
  SkinComponent()
    : mode_{ SkinningMode::LinearBlending }
    , skeleton_{ nullptr }
  {}

  ~SkinComponent();

  void update(float global_time);

  // void debug_draw(); //

  inline void set_skinning_mode(SkinningMode mode) {
    mode_ = mode;
  }

  inline void set_skeleton(SkeletonHandle skeleton) {
    skeleton_ = skeleton;
  }

  inline SkinningMode skinning_mode() const {
    return mode_;
  }

  inline SkeletonHandle skeleton() const {
    return skeleton_;
  }

  inline Sequence_t& sequence() {
    return sequence_;
  }

  inline SkeletonController const& controller() {
    return controller_;
  }

  inline uint32_t get_texture_id() const { return texture_id_; } // tmp

 private:
  // Inputs.
  SkinningMode    mode_       = SkinningMode::DualQuaternion;
  SkeletonHandle  skeleton_   = nullptr;
  Sequence_t      sequence_; //

  // Outputs.
  SkeletonController controller_;

  uint32_t buffer_id_  = 0;
  uint32_t texture_id_ = 0;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_COMPONENTS_SKIN_H_