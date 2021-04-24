#ifndef BARBU_ECS_MATERIALS_GENERIC_H_
#define BARBU_ECS_MATERIALS_GENERIC_H_

#include "ecs/material.h"
#include "memory/assets/texture.h"

#include "shaders/generic/interop.h"

// ----------------------------------------------------------------------------

class GenericMaterial : public Material {
 public:
  enum class ColorMode {
    Unlit         = MATERIAL_GENERIC_COLOR_MODE_UNLIT,
    Normal        = MATERIAL_GENERIC_COLOR_MODE_NORMAL,
    TexCoord      = MATERIAL_GENERIC_COLOR_MODE_TEXCOORD,
    Irradiance    = MATERIAL_GENERIC_COLOR_MODE_IRRADIANCE,
    LightPBR      = MATERIAL_GENERIC_COLOR_MODE_LIGHT_PBR,
    kCount,
    kInternal,
  };

  static constexpr ColorMode kDefaultColorMode{ ColorMode::LightPBR };
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
    //auto const cm = (color_mode == ColorMode::kInternal) ? color_mode_ : color_mode;
    bool const hasAlbedo     = static_cast<bool>(tex_albedo_);
    bool const hasMetalRough = static_cast<bool>(tex_metal_rough_);
    
    gx::SetUniform( pgm, "uColorMode",      static_cast<int32_t>(color_mode_));
    gx::SetUniform( pgm, "uColor",          color_);
    gx::SetUniform( pgm, "uAlphaCutOff",    cutoff);
    gx::SetUniform( pgm, "uMetallic",       metallic_);
    gx::SetUniform( pgm, "uRoughness",      roughness_);

    gx::SetUniform( pgm, "uHasAlbedo",      hasAlbedo);
    gx::SetUniform( pgm, "uHasMetalRough",  hasMetalRough);

    int32_t image_unit = 0;
    if (hasAlbedo) {
      gx::BindTexture( tex_albedo_->id,     image_unit);
      gx::SetUniform( pgm, "uAlbedoTex",    image_unit);
      ++image_unit;
    }
    if (hasMetalRough) {
      gx::BindTexture( tex_metal_rough_->id,  image_unit);
      gx::SetUniform( pgm, "uMetalRoughTex",  image_unit);
      ++image_unit;
    }
  }

 private:
  ColorMode     color_mode_;
  
  glm::vec4     color_;
  float         alpha_cutoff_;
  float         metallic_;
  float         roughness_;
  
  TextureHandle tex_albedo_;
  TextureHandle tex_metal_rough_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIALS_GENERIC_H_
