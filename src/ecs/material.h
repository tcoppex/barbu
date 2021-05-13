#ifndef BARBU_ECS_MATERIAL_H_
#define BARBU_ECS_MATERIAL_H_

#include "glm/glm.hpp"

#include "memory/assets/assets.h" //
#include "memory/enum_array.h"
#include "ui/ui_view.h"

// ----------------------------------------------------------------------------

enum class RenderMode {
  Opaque,
  Transparent,
  CutOff,
  kCount,
  kDefault = RenderMode::Opaque
};

// Attributes shared by all materials.
struct RenderAttributes {
  glm::mat4 world_matrix;
  glm::mat4 mvp_matrix;

  uint32_t skinning_texid = 0u;
  SkinningMode skinning_mode;

  glm::mat4 const* irradiance_matrices = nullptr;
  glm::vec3 eye_position;
  //int32_t tonemap_mode;
};

// ----------------------------------------------------------------------------

class Material {
 public:
  Material() = default;

  Material(AssetId program_id, RenderMode render_mode = RenderMode::kDefault)
    : program_id_(program_id)
    , render_mode_(render_mode)
    , image_unit_(0u)
    , bDoubleSided_(false)
    , last_image_unit_(0u)
  {}

  virtual ~Material() {
    if (ui_view_) {
      delete ui_view_;
      ui_view_ = nullptr;
    }
  }

  // Set internal data from material loading info.
  virtual void setup(MaterialInfo const& info) = 0;

  // Update commons uniforms for the material before rendering.
  void update_uniforms(RenderAttributes const& attributes, bool bUseSameProgram = false) {
    if (!bUseSameProgram) {
      auto pgm_handle = program();
      if (!pgm_handle) {
        LOG_FATAL_ERROR( "Material program \"", program_id_.c_str(), "\" not found." );
      }

      auto const pgm = pgm_handle->id;
      gx::UseProgram( pgm );
      
      last_image_unit_ = 0;

      // (vertex)
      gx::SetUniform( pgm, "uModelMatrix",        attributes.world_matrix);
      gx::SetUniform( pgm, "uMVP",                attributes.mvp_matrix);

      // (vertex skinning)
      if (attributes.skinning_texid > 0u) {
        gx::BindTexture( attributes.skinning_texid, last_image_unit_);
        gx::SetUniform( pgm, "uSkinningDatas",      last_image_unit_);    
        ++last_image_unit_;

        // [clean]
        skinning_subroutines_[ SkinningMode::LinearBlending ] = glGetSubroutineIndex(pgm, GL_VERTEX_SHADER, "skinning_LBS");
        skinning_subroutines_[ SkinningMode::DualQuaternion ] = glGetSubroutineIndex(pgm, GL_VERTEX_SHADER, "skinning_DQBS");
        auto const& su_index = skinning_subroutines_[ attributes.skinning_mode ];
        LOG_CHECK( su_index != GL_INVALID_INDEX );
        glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &su_index);
      }

      // (fragment)
      gx::SetUniform( pgm, "uIrradianceMatrices", attributes.irradiance_matrices, 3);
      gx::SetUniform( pgm, "uEyePosWS",           attributes.eye_position);
      //gx::SetUniform(pgm, "uToneMapMode",       static_cast<int>(attributes.tonemap_mode));
    
      CHECK_GX_ERROR();
    }
    image_unit_ = last_image_unit_;

    update_internals();
  }

  inline ProgramHandle program() {
    return PROGRAM_ASSETS.get(program_id_);
  }

  inline RenderMode render_mode() const {
    return render_mode_;
  }

  inline bool double_sided() const {
    return bDoubleSided_;
  }

  inline UIView* ui_view() {
    return ui_view_;
  }

  inline void set_render_mode(RenderMode const& render_mode) {
    render_mode_ = render_mode;
  }

  inline void set_double_sided(bool status) {
    bDoubleSided_ = status;
  }

 protected:
  virtual void update_internals() = 0;

  AssetId    program_id_;
  RenderMode render_mode_;

  EnumArray< uint32_t, SkinningMode > skinning_subroutines_; //
  int32_t image_unit_; //
  bool bDoubleSided_; //

 private:
  int32_t last_image_unit_;

 public:
  UIView *ui_view_ = nullptr; // [not used yet]
};

// ----------------------------------------------------------------------------

using MaterialHandle = std::shared_ptr<Material>;

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIAL_H_
