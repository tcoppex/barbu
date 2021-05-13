#include "ecs/components/skin.h"

#include "glm/gtc/type_ptr.hpp"
#include "core/graphics.h"
// #include "utils/gizmo.h" // tmp

// ----------------------------------------------------------------------------

SkinComponent::~SkinComponent() {
  if (buffer_id_ != 0u) {
    glDeleteBuffers(1u, &buffer_id_);
    buffer_id_ = 0u;

    glDeleteTextures(1u, &texture_id_);
    texture_id_ = 0u;
  }
}

// ----------------------------------------------------------------------------

void SkinComponent::update(float global_time) {
  if (skeleton_ == nullptr) {
    LOG_WARNING( "A skeleton was not provided for SkinComponent." );
    return;
  }

  // [optional] Use a blend tree to weight the sequence clips.
#if 0
  // Activates each sequence's leaves on the blend tree.
  // [TODO: set once by the Action State Machine each time a state is entered]
  blend_tree_->activate_leaves(true, sequence_); //

  // Compute weight for active leaves.
  blend_tree_->evaluate(1.0f, sequence_);
#endif

  // Evaluate the skinning data for the given sequence.
  if (controller_.evaluate( mode_, skeleton_, global_time, sequence_))
  {    
    int32_t bytesize(0);
    float const*data_ptr(nullptr);

    if (SkinningMode::DualQuaternion == mode_) {
      // DUAL QUATERNION BLENDING
      auto *const data = controller_.dual_quaternions().data();
      bytesize = skeleton_->njoints() * sizeof(data[0]);
      data_ptr = reinterpret_cast<float const*>(data);
    } else {
      // LINEAR BLENDING
      auto const& skinning = controller_.skinning_matrices();
      bytesize = skeleton_->njoints() * sizeof(skinning[0]);
      data_ptr = glm::value_ptr(skinning[0]);
    }

    // [ use an external wrapper, eg. 'TextureBuffer' ]
    {
      // Create an *immutable* device buffer with texture buffer.
      if (buffer_id_ == 0) {
        glCreateBuffers(1u, &buffer_id_);

        // Upper boundary of skinning data.
        int32_t constexpr kElemsize{ glm::max(sizeof(glm::mat3x4), sizeof(glm::dualquat)) };
        int32_t const kBytesize{ skeleton_->njoints() * kElemsize };
        glNamedBufferStorage(buffer_id_, kBytesize, nullptr, GL_DYNAMIC_STORAGE_BIT);
        
        glCreateTextures( GL_TEXTURE_BUFFER, 1u, &texture_id_);
        glTextureBuffer(texture_id_, GL_RGBA32F, buffer_id_);
      }

      // Upload skinning data.
      glNamedBufferSubData(buffer_id_, 0, bytesize, data_ptr);
      
      CHECK_GX_ERROR();
    }
  } else {
    LOG_WARNING( "Skinning was not computed : check for missing data." );
  }
}

// void SkinComponent::debug_draw() {
//   for (int i=0; i < skeleton_->njoints(); ++i) {
//     auto const& global = controller_.global_pose_matrices()[i];

//     auto const pos = glm::vec3(global[3]);
//     auto const end = glm::vec3(global * glm::vec4(0.0, 0.2, 0.0, 1.0));

//     float const di = i / float(skeleton_->njoints()-1);
//     Im3d::PushColor( Im3d::Color(glm::vec4(0.7f*di, 0.2f, 1.0f, 1.0f)) );
//     Im3d::DrawPrism( pos, end, 0.05, 32);
//     Im3d::PopColor();
//   }
// }

// ----------------------------------------------------------------------------
