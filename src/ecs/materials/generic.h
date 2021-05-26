#ifndef BARBU_ECS_MATERIALS_GENERIC_H_
#define BARBU_ECS_MATERIALS_GENERIC_H_

#include "ecs/material.h"
#include "memory/assets/texture.h"

#include "shaders/generic/interop.h"

// ----------------------------------------------------------------------------

class GenericMaterial : public Material {
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
    : Material( "GenericMaterial", render_mode)
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
    PROGRAM_ASSETS.create( program_id_, { 
      SHADERS_DIR "/generic/vs_generic.glsl",
      SHADERS_DIR "/generic/fs_generic.glsl"
    });
  }

  void setup(MaterialInfo const& info) override {
    if (info.bUnlit) {
      color_mode_ = ColorMode::Unlit;
    }
    bDoubleSided_ = info.bDoubleSided;
    
    color_        = info.diffuse_color;
    alpha_cutoff_ = info.alpha_cutoff;
    roughness_    = info.roughness;
    metallic_     = info.metallic;
    
    emissive_factor_ = info.emissive_factor;

    auto get_texture = [](auto const& str) {
      return (!str.empty()) ? TEXTURE_ASSETS.create2d( AssetId(str) ) : nullptr;
    };  

    tex_albedo_      = get_texture(info.diffuse_map);
    tex_normal_      = get_texture(info.bump_map);
    tex_rough_metal_ = get_texture(info.metallic_rough_map);
    tex_ao_          = get_texture(info.ao_map);
    tex_emissive_    = get_texture(info.emissive_map);

    // Switch mode depending on parameters.
    if (render_mode_ == RenderMode::kDefault) {
      if (info.bBlending) {
        render_mode_ = RenderMode::Transparent;
      } else if (info.bAlphaTest) {
        render_mode_ = RenderMode::CutOff;
      }
    }
  }

  void update_internals() final {
    auto handle = program();
    auto const pgm = handle->id;

    float const cutoff = (render_mode() == RenderMode::CutOff) ? alpha_cutoff_ : 0.0f;
    
    auto const cm = static_cast<int32_t>(color_mode_); 
    //(color_mode == ColorMode::kInternal) ? color_mode_ : color_mode;
    
    bool const hasAlbedo      = static_cast<bool>(tex_albedo_);
    bool const hasNormal      = static_cast<bool>(tex_normal_);
    bool const hasRoughMetal  = static_cast<bool>(tex_rough_metal_);
    bool const hasAO          = static_cast<bool>(tex_ao_);
    bool const hasEmissive    = static_cast<bool>(tex_emissive_);
    
    auto bind_texture = [this, &pgm](auto const& name, TextureHandle tex, gx::SamplerName sampler = gx::kDefaultSampler) {
      if (nullptr != tex) {
        gx::BindTexture( tex->id, texture_unit_, sampler);
        gx::SetUniform( pgm, name,  texture_unit_);
        ++texture_unit_;
      }
    };

    gx::SetUniform( pgm, "uColorMode",      cm);
    gx::SetUniform( pgm, "uColor",          color_);
    gx::SetUniform( pgm, "uAlphaCutOff",    cutoff);
    gx::SetUniform( pgm, "uMetallic",       metallic_);
    gx::SetUniform( pgm, "uRoughness",      roughness_);
    gx::SetUniform( pgm, "uEmissiveFactor", emissive_factor_);

    gx::SetUniform( pgm, "uHasAlbedo",      hasAlbedo);
    gx::SetUniform( pgm, "uHasNormal",      hasNormal);
    gx::SetUniform( pgm, "uHasRoughMetal",  hasRoughMetal);
    gx::SetUniform( pgm, "uHasAO",          hasAO);
    gx::SetUniform( pgm, "uHasEmissive",    hasEmissive);

    bind_texture( "uAlbedoTex",     tex_albedo_);
    bind_texture( "uNormalTex",     tex_normal_);
    bind_texture( "uRoughMetalTex", tex_rough_metal_);
    bind_texture( "uAOTex",         tex_ao_);
    bind_texture( "uEmissiveTex",   tex_emissive_);

    CHECK_GX_ERROR();
  }

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
