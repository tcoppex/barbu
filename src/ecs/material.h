#ifndef BARBU_ECS_MATERIAL_H_
#define BARBU_ECS_MATERIAL_H_

#include "glm/glm.hpp"

#include "memory/assets/assets.h" //
#include "ui/ui_view.h"

// ----------------------------------------------------------------------------

enum class RenderMode {
  Opaque,
  Transparent,
  kCount,
  kDefault = RenderMode::Opaque
};

// Attributes shared by all materials.
struct RenderAttributes {
  glm::mat4 const* irradiance_matrices;
  glm::mat4 world_matrix;
  glm::mat4 mvp_matrix;
  glm::vec3 eye_position;
};

// ----------------------------------------------------------------------------

class Material {
 public:
  Material() = default;

  Material(AssetId program_id, RenderMode render_mode = RenderMode::kDefault) : 
    program_id_(program_id), 
    render_mode_(render_mode)
  {}

  virtual ~Material() {
    if (ui_view_) {
      delete ui_view_;
      ui_view_ = nullptr;
    }
  }

  // Set internal data from material loading info.
  virtual void setup(MaterialInfo const& info) = 0;

  // Update uniforms for the material before rendering.
  void update_uniforms(RenderAttributes const& attributes, bool bUseSameProgram = false) {
    if (!bUseSameProgram) {
      auto pgm_handle = program();
      if (!pgm_handle) {
        LOG_FATAL_ERROR( "Material program \"", program_id_.c_str(), "\" not found." );
      }

      auto const pgm = pgm_handle->id;
      gx::UseProgram( pgm );

      gx::SetUniform( pgm, "uIrradianceMatrices", attributes.irradiance_matrices, 3);
      gx::SetUniform( pgm, "uModelMatrix",        attributes.world_matrix);
      gx::SetUniform( pgm, "uMVP",                attributes.mvp_matrix);
      gx::SetUniform( pgm, "uEyePosWS",           attributes.eye_position);
    }

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

 protected:
  virtual void update_internals() = 0;

  AssetId    program_id_;
  RenderMode render_mode_;
  bool bDoubleSided_ = false;

 public:
  UIView *ui_view_ = nullptr; // [not used yet]
};

// ----------------------------------------------------------------------------

using MaterialHandle = std::shared_ptr<Material>;

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIAL_H_
