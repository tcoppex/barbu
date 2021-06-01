#ifndef BARBU_ECS_MATERIAL_H_
#define BARBU_ECS_MATERIAL_H_

#include "memory/assets/assets.h"
#include "memory/enum_array.h"

// ----------------------------------------------------------------------------

enum class RenderMode {
  Opaque,
  Transparent,
  CutOff,
  kCount,

  kDefault = RenderMode::Opaque
};

// ----------------------------------------------------------------------------

// Attributes shared by all materials.
struct RenderAttributes {
  // (vertex)
  glm::mat4 mvp_matrix;
  glm::mat4 world_matrix;

  // (vertex skinning)
  uint32_t skinning_texid = 0u;
  SkinningMode skinning_mode;

  // (fragment)
  uint32_t brdf_lut_texid     = 0u;
  uint32_t prefilter_texid    = 0u;
  uint32_t irradiance_texid   = 0u;
  glm::mat4 const* irradiance_matrices = nullptr;
  glm::vec3 eye_position;
  //int32_t tonemap_mode;
};

// ----------------------------------------------------------------------------

class Material {
 public:
  Material() = default;

  Material(RenderMode render_mode = RenderMode::kDefault)
    : render_mode_(render_mode)
    , program_(nullptr)
    , texture_unit_(0u)
    , bDoubleSided_(false)
  {
    //program_ = PROGRAM_ASSETS.get( program_id ); //
  }

  virtual ~Material() {}

  // Set internal data from material loading info.
  virtual void setup(MaterialInfo const& info) = 0;

  // Update commons uniforms for the material before rendering.
  int32_t update_uniforms(RenderAttributes const& attributes, int32_t default_unit = 0);

  inline ProgramHandle program() {
    return program_; //
  }

  inline RenderMode render_mode() const noexcept {
    return render_mode_;
  }

  inline bool double_sided() const noexcept {
    return bDoubleSided_;
  }

  inline void set_render_mode(RenderMode const& render_mode) noexcept {
    render_mode_ = render_mode;
  }

  inline void set_double_sided(bool status) noexcept {
    bDoubleSided_ = status;
  }

 protected:
  virtual void update_internals() = 0;

  RenderMode render_mode_;
  ProgramHandle program_;

  // [to improve]
  EnumArray< uint32_t, SkinningMode > skinning_subroutines_;
  int32_t texture_unit_;
  bool bDoubleSided_;
};

// ----------------------------------------------------------------------------

// [confusing name with MaterialAssetHandle]
using MaterialHandle = std::shared_ptr<Material>; //

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIAL_H_
