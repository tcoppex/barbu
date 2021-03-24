#ifndef BARBU_ECS_MATERIALS_GENERIC_H_
#define BARBU_ECS_MATERIALS_GENERIC_H_

#include "ecs/material.h"
#include "memory/assets/texture.h"

// ----------------------------------------------------------------------------

class GenericMaterial : public Material {
 public:
  enum class ColorMode {
    Unlit,
    Normal,
    KeyLights,
    Irradiance,
    kCount,
    kDefault = ColorMode::Irradiance
  };

  GenericMaterial(RenderMode render_mode = RenderMode::kDefault)
    : Material("generic", render_mode)
    , color_mode_(ColorMode::kDefault)
    , tex_albedo_(nullptr)
    , color_{1.0f, 1.0f, 1.0f, 0.75f}
  {
    PROGRAM_ASSETS.create( program_id_, { 
      SHADERS_DIR "/generic/vs_generic.glsl",
      SHADERS_DIR "/generic/fs_generic.glsl"
    });
  }

  void setup(MaterialInfo const& info) override {
    tex_albedo_ = (info.diffuse_map.empty()) ? nullptr : TEXTURE_ASSETS.create2d( AssetId(info.diffuse_map) );
    color_      = info.diffuse_color;

    // if (!info.alpha_map.empty() || (color_.a < 1.0f)) {
    //   set_render_mode(RenderMode::Transparent);
    // }
  }

  void update_internals() final {
    auto handle = program();
    auto const pgm = handle->id;

    gx::SetUniform( pgm, "uColorMode",    static_cast<int>(color_mode_));
    gx::SetUniform( pgm, "uColor",        color_);

    bool const hasAlbedo = static_cast<bool>(tex_albedo_);
    gx::SetUniform( pgm, "uHasAlbedo",    hasAlbedo);

    int32_t image_unit = 0;
    if (hasAlbedo) {
      gx::BindTexture( tex_albedo_->id,   image_unit);
      gx::SetUniform( pgm, "uAlbedoTex",  image_unit);
      ++image_unit;
    }
  }

 private:
  ColorMode     color_mode_;
  
  TextureHandle tex_albedo_;
  glm::vec4     color_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_ECS_MATERIALS_GENERIC_H_
