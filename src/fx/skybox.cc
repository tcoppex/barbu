#include "fx/skybox.h"

#include "core/graphics.h"
#include "core/camera.h"
#include "fx/irradiance_env_map.h"
#include "fx/probe.h"
#include "memory/assets/assets.h"    

// ----------------------------------------------------------------------------

// Used to debug visualize the convolution pass output.
static bool constexpr kVisualizeIrradianceMap = false;

// ----------------------------------------------------------------------------

void Skybox::init() {
  // Compute program to transform a spherical map to a cubemap.
  pgm_.cs_transform = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/skybox/cs_spherical_to_cubemap.glsl" 
  );

  // Render skybox.
  pgm_.render = PROGRAM_ASSETS.createRender(
    "skybox::render",
    SHADERS_DIR "/skybox/vs_skybox.glsl",
    SHADERS_DIR "/skybox/fs_skybox.glsl"
  );

  // [ set externally, inside a "ConvolutionProbe" ? ]
  // Convolution used to calculate the irradiance map.
  pgm_.convolution = PROGRAM_ASSETS.createRender(
    "skybox::convolution",
    SHADERS_DIR "/skybox/vs_skybox.glsl",
    SHADERS_DIR "/skybox/fs_convolution.glsl"
  );

  // [ use a generic one ]
  // Skybox mesh.
  cube_mesh_ = MESH_ASSETS.createCube();

  setup_texture(); //

  CHECK_GX_ERROR();
}

void Skybox::deinit() {
  cube_mesh_.reset(); //
  sky_map_.reset(); //
}

void Skybox::render(Camera const& camera) {
  render(RenderMode::Sky, camera);
}

// ----------------------------------------------------------------------------

void Skybox::render(RenderMode mode, Camera const& camera) {
  auto const pgm = (mode == RenderMode::Sky) ? pgm_.render->id : pgm_.convolution->id;

  // Scale the box and recenter it to the camera.
  // [ we do not need to scale as we clamp depth in the vertex shader to assure
  // maximum scaling, that's just show the alternative. ]
  glm::mat4 view = camera.view();
  view[3] = glm::vec4(glm::vec3(0.0f), view[3].w);
  auto const mvp = camera.proj() * glm::scale( view, glm::vec3(camera.far()));
  gx::SetUniform( pgm, "uMVP", mvp);

  auto const tex_id = (kVisualizeIrradianceMap && irradiance_map_) ? irradiance_map_->id : sky_map_->id;

  int32_t texture_unit = -1;
  gx::BindTexture( tex_id, ++texture_unit, gx::LinearClamp);
  gx::SetUniform( pgm, "uCubemap", texture_unit);

  gx::UseProgram(pgm);
    cube_mesh_->draw();
  gx::UseProgram(0u);

  gx::UnbindTexture();
  
  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Skybox::setup_texture(/*ResourceInfo info*/) {
  // Environment map.
  if constexpr (true) {
    // Crossed HDR map.

    ResourceInfo resource_info( 
      ASSETS_DIR "/textures/cross_hdr/uffizi_cross_mmp_s.hdr"
    );
    sky_map_ = TEXTURE_ASSETS.createCubemapHDR( resource_info.id );

    if (sky_map_ && sky_map_->loaded()) {
      IrradianceEnvMap::PrefilterHDR( resource_info, sh_matrices_);
    }
  } else {
    // Spherical HDR.

    constexpr int32_t kResolution = Probe::kDefaultCubemapResolution; //
    constexpr int32_t kNumFaces = Probe::kViewMatrices.size(); //
    constexpr uint32_t kFormat = GL_RGBA16F; //

    // Input spherical texture.
    auto spherical_tex = TEXTURE_ASSETS.create2d( 
      ASSETS_DIR "/textures/reinforced_concrete_02_1k.hdr",
      1, kFormat 
    ); //
    
    // Output sky cubemap.
    sky_map_ = TEXTURE_ASSETS.createCubemap( "skybox::Cubemap", 1, kFormat, kResolution, kResolution );

    // Transform spherical map to cubical.
    {
      auto const pgm = pgm_.cs_transform->id;  

      constexpr int32_t texture_unit = 0;
      gx::BindTexture( spherical_tex->id, texture_unit, gx::LinearRepeat);
      gx::SetUniform( pgm, "uSphericalTex", texture_unit);

      constexpr int32_t image_unit = 0;
      glBindImageTexture( image_unit, sky_map_->id, 0, GL_TRUE, 0, GL_WRITE_ONLY, kFormat); //
      gx::SetUniform( pgm, "uDstImg", image_unit);

      gx::SetUniform( pgm, "uResolution", kResolution);
      gx::SetUniform( pgm, "uFaceViews", Probe::kViewMatrices.data(), kNumFaces);

      gx::UseProgram(pgm);
        gx::DispatchCompute<16, 16>( kResolution, kResolution, kNumFaces);
      gx::UseProgram(0u);

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      // Unbind all.
      gx::UnbindTexture( texture_unit );
      glBindImageTexture( image_unit, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, kFormat);
    }
  }

  // Create the irradiance envmap.
  if constexpr(true) {
    constexpr int32_t kIrradianceMapResolution = 64;
    
    // ---
    // gx::Enable( gx::State::CubeMapSeamless );
    // gx::Disable( gx::State::CullFace );
    // ---

    Probe probe;
    probe.init(kIrradianceMapResolution, false);
    probe.capture( [this](Camera const& camera) {
      render( RenderMode::Convolution, camera); 
    });

    irradiance_map_ = probe.texture();
  }
}

// ----------------------------------------------------------------------------
