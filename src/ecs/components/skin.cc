#include "ecs/components/skin.h"

#include "glm/gtc/type_ptr.hpp"
#include "core/graphics.h"

// ----------------------------------------------------------------------------

SkinComponent::~SkinComponent() {
  if (0u != buffer_id_) {
    glDeleteBuffers(1u, &buffer_id_);
    buffer_id_ = 0u;

    glDeleteTextures(1u, &texture_id_);
    texture_id_ = 0u;
  }
}

bool SkinComponent::update(float global_time) {
  if (nullptr == skeleton_) {
    LOG_WARNING( "A skeleton was not provided for SkinComponent." );
    return false;
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
  bool const hasData{
    controller_.evaluate( mode_, skeleton_, global_time, sequence_)
  };

  if (hasData) {
    updateSkinningBuffer();
  }
  
  return hasData;
}

void SkinComponent::updateSkinningBuffer() {
  // [ TODO : use an external wrapper, eg. 'TextureBuffer' ]

  // 1) Create an *immutable* device buffer with texture buffer.
  //    [This is unique to each instance but they could share a larger one instead]
  if (0u == buffer_id_) {
    // Upper boundary for skinning data.
    GLsizeiptr constexpr kElemsize{ static_cast<GLsizeiptr>(glm::max(sizeof(glm::mat3x4), sizeof(glm::dualquat))) }; //
    GLsizeiptr const kBytesize{ skeleton_->njoints() * kElemsize };

    glCreateBuffers(1u, &buffer_id_);
    glNamedBufferStorage(buffer_id_, kBytesize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateTextures(GL_TEXTURE_BUFFER, 1u, &texture_id_);
    glTextureBuffer(texture_id_, GL_RGBA32F, buffer_id_);

    CHECK_GX_ERROR();
  }

  // 2) Retrieve the underlying data depending on the skinning blending mode.
  GLsizeiptr bytesize{0};
  float const* data_ptr{nullptr};
  
  if (SkinningMode::DualQuaternion == mode_) {
    // DUAL QUATERNION BLENDING.
    auto *const data = controller_.dual_quaternions().data();
    bytesize = skeleton_->njoints() * sizeof(data[0]);
    data_ptr = reinterpret_cast<float const*>(data);
  } else {
    // LINEAR BLENDING.
    auto const& data = controller_.skinning_matrices();
    bytesize = skeleton_->njoints() * sizeof(data[0]);
    data_ptr = glm::value_ptr(data[0]);
  }

  // 3) Upload skinning data.
  glNamedBufferSubData(buffer_id_, 0, bytesize, data_ptr);
  
  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
