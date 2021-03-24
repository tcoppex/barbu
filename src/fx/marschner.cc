#include "fx/marschner.h"

#include "core/graphics.h"
#include "memory/assets/assets.h"
#include "ui/views/fx/MarschnerView.h"

// ----------------------------------------------------------------------------

Marschner::~Marschner() {
  glDeleteTextures( kNumLUTs, tex_.data()); //
}

void Marschner::init() {

  // 1) Create the CS programs.
  pgm_[0] = PROGRAM_ASSETS.create( SHADERS_DIR "/hair/marschner/cs_marschner_m.glsl" )->id;  
  pgm_[1] = PROGRAM_ASSETS.create( SHADERS_DIR "/hair/marschner/cs_marschner_n.glsl" )->id;

  // Setup textures.
  for (int i=0; i<kNumLUTs; ++i) {
    auto &tex = tex_[i];
    glCreateTextures(GL_TEXTURE_2D, 1u, &tex);
    glTextureStorage2D(tex, 1, kTextureFormat, kTextureResolution, kTextureResolution);

    glTextureParameteri( tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri( tex, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureParameteri( tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri( tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
  }
  CHECK_GX_ERROR();

  // Setup the UI.
  ui_view_ = new views::MarschnerView(params_);
}

// ----------------------------------------------------------------------------

void Marschner::update(bool bForceUpdate) {
  if (bForceUpdate || !(lastShadingParams_ == params_.shading)) {
    generate();
  }

  lastShadingParams_ = params_.shading;
}

// ----------------------------------------------------------------------------

void Marschner::generate() {
  auto const inv_resolution = 1.0f / kTextureResolution;

  for (int i = 0; i < kNumLUTs; ++i) {
    auto &tex = tex_[i];

    // Bind destination texture.    
    glBindImageTexture( 0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, kTextureFormat); //

    // Launch compute program.
    auto &pgm = pgm_[i];

    if (i == 0) {
      gx::SetUniform( pgm, "uLongitudinalShift",  params_.shading.ar);
      gx::SetUniform( pgm, "uLongitudinalWidth",  params_.shading.br);
    } else {
      gx::SetUniform( pgm, "uEta",            params_.shading.eta);
      gx::SetUniform( pgm, "uAbsorption",     params_.shading.absorption);
      gx::SetUniform( pgm, "uEccentricity",   params_.shading.eccentricity);
      gx::SetUniform( pgm, "uGlintScale",     params_.shading.glintScale);
      gx::SetUniform( pgm, "uDeltaCaustic",   params_.shading.deltaCaustic);
      gx::SetUniform( pgm, "uDeltaHm",        params_.shading.deltaHm);
    }
    gx::SetUniform( pgm, "uInvResolution",      inv_resolution);
    gx::SetUniform( pgm, "uDstImg",             0);

    gx::UseProgram(pgm);
    gx::DispatchCompute<kComputeBlockSize, kComputeBlockSize>(kTextureResolution, kTextureResolution);

    // Synchronize memory.
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT); //
  
    params_.tex_ptr[i] = &tex; //
  }
  gx::UseProgram();

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
