
#include "fx/skybox.h"

#include "core/graphics.h"
#include "core/camera.h"
#include "fx/irradiance_env_map.h"
#include "memory/assets/assets.h"    

// ----------------------------------------------------------------------------

void Skybox::init() {
  cubemesh_ = MESH_ASSETS.createCube();

  // Shader program.
  pgm_ = PROGRAM_ASSETS.createRender(
    "skybox",
    SHADERS_DIR "/skybox/vs_skybox.glsl",
    SHADERS_DIR "/skybox/fs_skybox.glsl"
  )->id;

  // Cubemap.
  {
#if 1
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
#else
    // Handle crossed HDR only.
    ResourceInfo res( ASSETS_DIR "/textures/cross_hdr/grace_cross_mmp_s.hdr" );
    skytex_ = TEXTURE_ASSETS.createCubemapHDR( res.id );

    // [ Irradiance is incorrectly compute with hdr values ].
    if (skytex_ && skytex_->loaded()) {
      //IrradianceEnvMap::PrefilterHDR( res, shMatrices_);
    }
#endif
  }

  CHECK_GX_ERROR();
}

void Skybox::deinit() {
  cubemesh_.reset();
  skytex_.reset();
}

void Skybox::render(Camera const& camera) {
  // Scale the box and recenter it to the camera.
  glm::mat4 model = camera.view();
  model[3] = glm::vec4(glm::vec3(0.0f), model[3].w);
  
  auto const mvp = camera.proj()
                 * glm::scale(model, glm::vec3(camera.far()))
                 ;
  gx::SetUniform( pgm_, "uMVP", mvp);

  gx::BindTexture( skytex_->id );
  gx::SetUniform( pgm_, "uCubemap", 0);

  gx::UseProgram(pgm_);
  cubemesh_->draw();
  gx::UseProgram(0u);
  
  gx::UnbindTexture();
  
  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
