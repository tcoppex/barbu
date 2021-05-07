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
    Metallic      = MATERIAL_GENERIC_COLOR_MODE_METALLIC,
    Roughness     = MATERIAL_GENERIC_COLOR_MODE_ROUGHNESS,
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
    , metallic_(0.0f)
    , roughness_(0.4f)
    , tex_albedo_(nullptr)
    , tex_metal_rough_(nullptr)
    , tex_ao_(nullptr)
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
    metallic_     = info.metallic;
    roughness_    = info.roughness;

    auto get_texture = [](auto const& str) {
      return (!str.empty()) ? TEXTURE_ASSETS.create2d( AssetId(str) ) : nullptr;
    };  

    tex_albedo_      = get_texture(info.diffuse_map);
    tex_metal_rough_ = get_texture(info.metallic_rough_map);
    tex_ao_          = get_texture(info.ao_map);

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
    bool const hasMetalRough  = static_cast<bool>(tex_metal_rough_);
    bool const hasAO          = static_cast<bool>(tex_ao_);
    
    auto bind_texture = [this, &pgm](auto const& name, TextureHandle tex) {
      if (nullptr != tex) {
        gx::BindTexture( tex->id, image_unit_ /*, material_sampler*/);
        gx::SetUniform( pgm, name,  image_unit_);
        ++image_unit_;
      }
    };

    gx::SetUniform( pgm, "uColorMode",      cm);
    gx::SetUniform( pgm, "uColor",          color_);
    gx::SetUniform( pgm, "uAlphaCutOff",    cutoff);
    gx::SetUniform( pgm, "uMetallic",       metallic_);
    gx::SetUniform( pgm, "uRoughness",      roughness_);

    gx::SetUniform( pgm, "uHasAlbedo",      hasAlbedo);
    gx::SetUniform( pgm, "uHasMetalRough",  hasMetalRough);
    gx::SetUniform( pgm, "uHasAO",          hasAO);
  
    bind_texture( "uAlbedoTex",     tex_albedo_);
    bind_texture( "uMetalRoughTex", tex_metal_rough_);
    bind_texture( "uAOTex",         tex_ao_);

    CHECK_GX_ERROR();
  }

 private:
  ColorMode     color_mode_;
  
  glm::vec4     color_;
  float         alpha_cutoff_;
  float         metallic_;
  float         roughness_;
  
  TextureHandle tex_albedo_;
  TextureHandle tex_metal_rough_;
  TextureHandle tex_ao_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIALS_GENERIC_H_
