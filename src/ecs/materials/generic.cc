#include "ecs/materials/generic.h"

// ----------------------------------------------------------------------------

void GenericMaterial::setup(MaterialInfo const& info) {
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

void GenericMaterial::updateInternals() {
  auto const pgm = program()->id;

  float const cutoff = (renderMode() == RenderMode::CutOff) ? alpha_cutoff_ : 0.0f;
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

// ----------------------------------------------------------------------------