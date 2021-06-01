#include "ecs/material.h"

#include "glm/glm.hpp"
#include "core/graphics.h"
#include "core/logger.h"

// ----------------------------------------------------------------------------

int32_t Material::update_uniforms(RenderAttributes const& attributes, int32_t default_unit) {
  texture_unit_ = default_unit; //

  bool const use_new_program{ texture_unit_ == 0 };

  if (use_new_program) {
    auto const pgm = program_->id;
    gx::UseProgram( pgm );

    auto bind_texture = [this, &pgm](auto const& name, uint32_t id, gx::SamplerName samplerName = gx::SamplerName::kDefaultSampler) {
      if (id > 0u) {
        gx::BindTexture( id, texture_unit_, samplerName);
        gx::SetUniform( pgm, name, texture_unit_);
        ++texture_unit_;  
      }
    };

    // (vertex)
    gx::SetUniform( pgm, "uMVP",         attributes.mvp_matrix);
    gx::SetUniform( pgm, "uModelMatrix", attributes.world_matrix);

    // (vertex skinning)
    if (attributes.skinning_texid > 0u) {
      bind_texture( "uSkinningDatas",   attributes.skinning_texid,  gx::SamplerName::LinearClamp);
      
      // [ improve ]
      skinning_subroutines_[ SkinningMode::LinearBlending ] = glGetSubroutineIndex(pgm, GL_VERTEX_SHADER, "skinning_LBS");
      skinning_subroutines_[ SkinningMode::DualQuaternion ] = glGetSubroutineIndex(pgm, GL_VERTEX_SHADER, "skinning_DQBS");
      auto const& su_index = skinning_subroutines_[ attributes.skinning_mode ];
      LOG_CHECK( su_index != GL_INVALID_INDEX );
      glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &su_index);
    }

    // (fragment)
    bind_texture( "uBRDFMap",           attributes.brdf_lut_texid,   gx::SamplerName::LinearClamp);
    bind_texture( "uPrefilterEnvmap",   attributes.prefilter_texid,  gx::SamplerName::LinearMipmapClamp);
    bind_texture( "uIrradianceEnvmap",  attributes.irradiance_texid, gx::SamplerName::LinearClamp);

    bool const has_irradiance_matrices = (nullptr != attributes.irradiance_matrices);
    gx::SetUniform( pgm, "uHasIrradianceMatrices", has_irradiance_matrices);
    if (has_irradiance_matrices) {
      gx::SetUniform( pgm, "uIrradianceMatrices", attributes.irradiance_matrices, 3);
    }

    gx::SetUniform( pgm, "uEyePosWS", attributes.eye_position);
    //gx::SetUniform(pgm, "uToneMapMode",       static_cast<int>(attributes.tonemap_mode));
  }
  CHECK_GX_ERROR();

  auto const last_unit = texture_unit_;
  update_internals();
  texture_unit_ = last_unit;

  return texture_unit_;
}

// ----------------------------------------------------------------------------