#include "fx/skybox.h"

#include "core/graphics.h"
#include "core/camera.h"
#include "fx/irradiance_env_map.h"
#include "fx/probe.h"
#include "memory/assets/assets.h"    

// ----------------------------------------------------------------------------

void Skybox::init() {
  cubemesh_ = MESH_ASSETS.createCube();

  // Shader program.
  pgm_ = PROGRAM_ASSETS.createRender(
    "skybox",
    SHADERS_DIR "/skybox/vs_skybox.glsl",
    SHADERS_DIR "/skybox/fs_skybox.glsl"
  );

  // Cubemap.
#if 0
  // LDR maps.

  ResourceInfoList const dependencies( {
    ASSETS_DIR "/textures/cubemap/Lycksele2/posx.jpg",
    ASSETS_DIR "/textures/cubemap/Lycksele2/negx.jpg",
    ASSETS_DIR "/textures/cubemap/Lycksele2/posy.jpg",
    ASSETS_DIR "/textures/cubemap/Lycksele2/negy.jpg",
    ASSETS_DIR "/textures/cubemap/Lycksele2/posz.jpg",
    ASSETS_DIR "/textures/cubemap/Lycksele2/negz.jpg",
  } );

  skytex_ = TEXTURE_ASSETS.createCubemap( "skybox_cubemap", dependencies);

  if (skytex_ && skytex_->loaded()) {
    // Prefilter Irradiance env map on CPU (costly, to port to compute shaders).
    IrradianceEnvMap::PrefilterU8( dependencies, shMatrices_);
  }
#elif 1
  // Crossed HDR map.

  ResourceInfo res( 
    //ASSETS_DIR "/textures/cross_hdr/grace_cross_mmp_s.hdr"
    ASSETS_DIR "/textures/cross_hdr/uffizi_cross_mmp_s.hdr" 
    //ASSETS_DIR "/textures/cross_hdr/grace_new_cross.hdr"
  );

  skytex_ = TEXTURE_ASSETS.createCubemapHDR( res.id );

  // !! HDR irradiance matrices needs to be scaled down per cubemap.
  if (skytex_ && skytex_->loaded()) {
    IrradianceEnvMap::PrefilterHDR( res, shMatrices_);
  }
#else
  // Spherical HDR.

  constexpr int32_t kResolution = 512; //
  constexpr int32_t kNumFaces = 6; //

  // Compute program to transform a spherical map to a cubemap.
  pgm_transform_ = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/skybox/cs_spherical_to_cubemap.glsl" 
  );
  auto const pgm = pgm_transform_->id;  

  // Input spherical map (temporary).
  auto spherical_tex = TEXTURE_ASSETS.create2d( 
    ASSETS_DIR "/textures/reinforced_concrete_02_1k.hdr", 
    //ASSETS_DIR "/textures/de_balie_2k.hdr", 
    
    1, GL_RGBA16F 
  ); //
  
  // Output skybox texture.
  skytex_ = TEXTURE_ASSETS.createCubemap( "SkyboxCubemap", 1, GL_RGBA16F, kResolution, kResolution );

  constexpr int32_t texture_unit = 0;
  gx::BindTexture( spherical_tex->id, texture_unit, gx::LinearRepeat);
  gx::SetUniform( pgm, "uSphericalTex", texture_unit);

  constexpr int32_t image_unit = 0;
  glBindImageTexture( image_unit, skytex_->id, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F); //
  gx::SetUniform( pgm, "uDstImg",       image_unit);

  gx::SetUniform( pgm, "uResolution",   kResolution);
  gx::SetUniform( pgm, "uFaceViews",    Probe::kViewMatrices.data(), kNumFaces);

  gx::UseProgram(pgm);
    gx::DispatchCompute<16, 16>( kResolution, kResolution, kNumFaces);
  gx::UseProgram(0u);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // Unbind all.
  gx::UnbindTexture( texture_unit );
  glBindImageTexture( image_unit, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
#endif

  // probe_.init();
  // probe_.capture([this](Camera const& camera) { this->render(camera); });

  CHECK_GX_ERROR();
}

void Skybox::deinit() {
  cubemesh_.reset(); //
  skytex_.reset(); //
  probe_.release();
}

void Skybox::render(Camera const& camera) {
  // Scale the box and recenter it to the camera.
  // [ we do not need to scale as we clamp depth in the vertex shader to assure
  // maximum scaling, that's just an alternative. ]
  glm::mat4 model = camera.view();
  model[3] = glm::vec4(glm::vec3(0.0f), model[3].w);
  auto const mvp = camera.proj()
                 * glm::scale(model, glm::vec3(camera.far()))
                 ;

  auto const pgm = pgm_->id;
  int32_t texunit = -1;

  gx::SetUniform( pgm, "uMVP", mvp);

  gx::BindTexture( skytex_->id, ++texunit, gx::LinearClamp);
  gx::SetUniform( pgm, "uCubemap", texunit);

  gx::UseProgram(pgm);
    cubemesh_->draw();
  gx::UseProgram(0u);

  gx::UnbindTexture();
  
  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
