#include "fx/marschner.h"

#include "memory/assets/assets.h"
#include "ui/views/fx/MarschnerView.h"

// ----------------------------------------------------------------------------

Marschner::~Marschner() {
}

void Marschner::init() {
  // Create the CS programs.
  programs_[0] = PROGRAM_ASSETS.create( SHADERS_DIR "/hair/marschner/cs_marschner_m.glsl" );  
  programs_[1] = PROGRAM_ASSETS.create( SHADERS_DIR "/hair/marschner/cs_marschner_n.glsl" );

  // Setup textures.
  textures_[0] = TEXTURE_ASSETS.create2d( "Marschner::M_LUT", 1, kTextureFormat, kTextureResolution, kTextureResolution);
  textures_[1] = TEXTURE_ASSETS.create2d( "Marschner::N_LUT", 1, kTextureFormat, kTextureResolution, kTextureResolution);

  // Setup the UI.
  ui_view = std::make_shared<views::MarschnerView>(params_);
}

// ----------------------------------------------------------------------------

void Marschner::update(bool bForceUpdate) {
  if (bForceUpdate || (previous_shading_params_ != params_.shading)) {
    generate();
  }
  previous_shading_params_ = params_.shading;
}

// ----------------------------------------------------------------------------

void Marschner::generate() {
  for (int i = 0; i < kNumLUTs; ++i) {
    auto const& tex = textures_[i]->id;

    // Bind destination texture.
    glBindImageTexture( 0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, kTextureFormat); //

    // Launch compute program.
    auto const pgm = programs_[i]->id;

    if (i == 0) {
      gx::SetUniform( pgm, "uLongitudinalShift",  params_.shading.ar);
      gx::SetUniform( pgm, "uLongitudinalWidth",  params_.shading.br);
    } else {
      gx::SetUniform( pgm, "uEta",                params_.shading.eta);
      gx::SetUniform( pgm, "uAbsorption",         params_.shading.absorption);
      gx::SetUniform( pgm, "uEccentricity",       params_.shading.eccentricity);
      gx::SetUniform( pgm, "uGlintScale",         params_.shading.glintScale);
      gx::SetUniform( pgm, "uDeltaCaustic",       params_.shading.deltaCaustic);
      gx::SetUniform( pgm, "uDeltaHm",            params_.shading.deltaHm);
    }
    gx::SetUniform( pgm, "uInvResolution",    kInvTextureResolution);
    gx::SetUniform( pgm, "uDstImg",           0);

    gx::UseProgram(pgm);
    gx::DispatchCompute<kComputeBlockSize, kComputeBlockSize>(kTextureResolution, kTextureResolution);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT); //

    // Put the texture pointer into params to be viewed by the UI.
    params_.tex_ptr[i] = &tex; //
  }
  gx::UseProgram();

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Marschner::bindLUTs(int baseUnit) {
  for (int i = 0; i < kNumLUTs; ++i) {
    gx::BindTexture(textures_[i]->id, baseUnit + i, gx::SamplerName::LinearRepeat);
  }
}

// ----------------------------------------------------------------------------

void Marschner::unbindLUTs(int baseUnit) {
  for (int i = 0; i < kNumLUTs; ++i) {
    gx::UnbindTexture(baseUnit + i);
  }
}

// ----------------------------------------------------------------------------
