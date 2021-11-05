#ifndef BARBU_ECS_MATERIALS_GENERIC_H_
#define BARBU_ECS_MATERIALS_GENERIC_H_

#include "ecs/material.h"
#include "memory/assets/texture.h"

#include "shaders/generic/interop.h"

// ----------------------------------------------------------------------------

class GenericMaterial final : public Material {
 public:
  enum class ColorMode {
    PBR           = MATERIAL_GENERIC_COLOR_MODE_PBR,
    Unlit         = MATERIAL_GENERIC_COLOR_MODE_UNLIT,
    Normal        = MATERIAL_GENERIC_COLOR_MODE_NORMAL,
    TexCoord      = MATERIAL_GENERIC_COLOR_MODE_TEXCOORD,
    Irradiance    = MATERIAL_GENERIC_COLOR_MODE_IRRADIANCE,
    AO            = MATERIAL_GENERIC_COLOR_MODE_AO,
    Roughness     = MATERIAL_GENERIC_COLOR_MODE_ROUGHNESS,
    Metallic      = MATERIAL_GENERIC_COLOR_MODE_METALLIC,
    kCount,
    kInternal,
  };

  static constexpr ColorMode kDefaultColorMode{ ColorMode::PBR };
  static constexpr glm::vec4 kDefaultColor{ 1.0f, 1.0f, 1.0f, 0.75f };
  static constexpr float kDefaultAlphaCutOff{ 0.5f };

  GenericMaterial(RenderMode render_mode = RenderMode::kDefault)
    : Material( render_mode ) //
    , color_mode_(kDefaultColorMode)
    , color_{kDefaultColor}
    , alpha_cutoff_(kDefaultAlphaCutOff)
    , roughness_(0.0f) //
    , metallic_(0.0f) //
    , emissive_factor_{1.0f, 1.0f, 1.0f}
    , tex_albedo_(nullptr)
    , tex_normal_(nullptr)
    , tex_rough_metal_(nullptr)
    , tex_ao_(nullptr)
    , tex_emissive_(nullptr)
  {
    program_ = PROGRAM_ASSETS.create( "Material::Generic", { 
      SHADERS_DIR "/generic/vs_generic.glsl",
      SHADERS_DIR "/generic/fs_generic.glsl"
    });
  }

  void setup(MaterialInfo const& info) final;
  void updateInternals() final;

 private:
  ColorMode     color_mode_;
  
  glm::vec4     color_;
  float         alpha_cutoff_;
  float         roughness_;
  float         metallic_;
  glm::vec3     emissive_factor_;
  
  TextureHandle tex_albedo_;
  TextureHandle tex_normal_;
  TextureHandle tex_rough_metal_;
  TextureHandle tex_ao_;
  TextureHandle tex_emissive_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIALS_GENERIC_H_
