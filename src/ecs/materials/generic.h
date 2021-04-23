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

  static constexpr ColorMode kDefaultColorMode{ ColorMode::Irradiance };
  static constexpr glm::vec4 kDefaultColor{ 1.0f, 1.0f, 1.0f, 0.75f };
  static constexpr float kDefaultAlphaCutOff{ 0.5f };

  GenericMaterial(RenderMode render_mode = RenderMode::kDefault)
    : Material( "GenericMaterial", render_mode)
    , color_mode_(kDefaultColorMode)
    , color_{kDefaultColor}
    , alpha_cutoff_(kDefaultAlphaCutOff)
    , tex_albedo_(nullptr)
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

    tex_albedo_   = (info.diffuse_map.empty()) ? nullptr 
                                               : TEXTURE_ASSETS.create2d( AssetId(info.diffuse_map) )
                                               ;

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

    //auto const cm = (color_mode == ColorMode::kInternal) ? color_mode_ : color_mode;
    gx::SetUniform( pgm, "uColorMode",   static_cast<int32_t>(color_mode_));

    float const cutoff = (render_mode() == RenderMode::CutOff) ? alpha_cutoff_ : 0.0f;
    
    gx::SetUniform( pgm, "uColor",       color_);
    gx::SetUniform( pgm, "uAlphaCutOff", cutoff);

    // Textures.
    int32_t image_unit = 0;
    bool const hasAlbedo = static_cast<bool>(tex_albedo_);
    if (hasAlbedo) {
      gx::BindTexture( tex_albedo_->id,  image_unit);
      gx::SetUniform( pgm, "uAlbedoTex", image_unit);
      ++image_unit;
    }
    gx::SetUniform( pgm, "uHasAlbedo",   hasAlbedo);
  }

 private:
  ColorMode     color_mode_;
  
  glm::vec4     color_;
  float         alpha_cutoff_;
  
  TextureHandle tex_albedo_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIALS_GENERIC_H_
