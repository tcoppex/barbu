#include "fx/skybox.h"

#include "core/graphics.h"
#include "core/camera.h"
#include "fx/probe.h"
#include "memory/assets/assets.h"    

// ----------------------------------------------------------------------------

// Used to debug visualize the convolution pass output.
static bool constexpr kVisualizeIrradianceMap = false;
static bool constexpr kVisualizeSpecularMap   = false;

// ----------------------------------------------------------------------------

void Skybox::init(ResourceId resource_id) {
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

  //
  pgm_.prefilter = PROGRAM_ASSETS.createRender(
    "skybox::prefilter",
    SHADERS_DIR "/skybox/vs_skybox.glsl",
    SHADERS_DIR "/skybox/fs_prefiltering.glsl"
  );

  // [ use a generic one ]
  // Skybox mesh.
  cube_mesh_ = MESH_ASSETS.createCube();

  setup_texture(resource_id); //

  CHECK_GX_ERROR();
}

void Skybox::deinit() {
  cube_mesh_.reset(); //
  sky_map_.reset(); //
  irradiance_map_.reset();
  specular_map_.reset();
}

void Skybox::render(Camera const& camera) {
  render(RenderMode::Sky, camera);
}


void Skybox::setup_texture(ResourceId resource_id) {
  assert(sky_map_ == nullptr);

  // (we might want to use mipmaps for late convolutions)
  constexpr int32_t kLevels = 1;

  auto const kSkyboxCubemapID = TEXTURE_ASSETS.findUniqueID( "Skybox::Cubemap" );
  auto const basename   = Resource::TrimFilename(resource_id);
  bool const is_crossed = basename.find("cross") != std::string::npos;

  // Environment sky map.
  if (is_crossed) {
    // -- Crossed HDR map --

    sky_map_ = TEXTURE_ASSETS.createCubemapHDR( kSkyboxCubemapID, kLevels, resource_id);

    if (sky_map_ && sky_map_->loaded()) {
      Irradiance::PrefilterHDR( ResourceInfo(resource_id), sh_matrices_);
      has_sh_matrices_ = true;
    }
  } else {
    // -- Spherical HDR --

    constexpr int32_t kResolution = Probe::kDefaultCubemapResolution; //
    constexpr int32_t kNumFaces   = Probe::kViewMatrices.size(); //
    constexpr uint32_t kFormat    = GL_RGBA16F; //

    // Input spherical texture [tmp].
    auto spherical_tex = TEXTURE_ASSETS.create2d( resource_id, 1, kFormat); //
    
    // Output sky cubemap.
    sky_map_ = TEXTURE_ASSETS.createCubemap( kSkyboxCubemapID, kLevels, kFormat, kResolution, kResolution ); //

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

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); //

      // Generate sub mips levels when needed.
      if (kLevels > 1) {
        sky_map_->generate_mipmaps();
      }

      // Unbind all.
      gx::UnbindTexture( texture_unit );
      glBindImageTexture( image_unit, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, kFormat); //
    }
  }

  // Prove use to generate the envmap convolutions.
  Probe probe;

  // Be sure to have set the proper pipeline states (Already done in the renderer).
  // gx::Enable( gx::State::CubeMapSeamless );
  // gx::Disable( gx::State::CullFace );

  // Irradiance envmap.
  if (!has_sh_matrices_) {
    constexpr int32_t kIrradianceMapResolution = 64;
    probe.setup( kIrradianceMapResolution, 1, false);
    probe.capture( [this](Camera const& camera, int32_t level) {
      render( RenderMode::Convolution, camera); 
    });
    irradiance_map_ = probe.texture();
  }
  LOG_DEBUG_INFO( "Skybox map", basename, "use",  has_sh_matrices_ ? "SH matrices." : "an irradiance map." );

  // --------------------------------------
  
  // [Work in Progress]
  // Specular envmap.
  if constexpr(true) {
    constexpr int32_t kSpecularMapNumSamples = 2048; //
    constexpr int32_t kSpecularMapResolution = 256; //
    constexpr int32_t kSpecularMapLevel      = Texture::GetMaxMipLevel(kSpecularMapResolution);
    constexpr float kInvMaxLevel             = 1.0f / (kSpecularMapLevel - 1.0f);

    pgm_.prefilter->setUniform( "uNumSamples", kSpecularMapNumSamples);
    probe.setup( kSpecularMapResolution, kSpecularMapLevel, false); //
    probe.capture( [this, kInvMaxLevel](Camera const& camera, int32_t level) {
      const float roughness = level * kInvMaxLevel;
      pgm_.prefilter->setUniform( "uRoughness",  roughness); //
      render( RenderMode::Prefilter, camera); 
    });
    specular_map_ = probe.texture();
  }
}


void Skybox::render(RenderMode mode, Camera const& camera) {
  auto const pgm = (mode == RenderMode::Sky)         ? pgm_.render->id : 
                   (mode == RenderMode::Convolution) ? pgm_.convolution->id
                                                     : pgm_.prefilter->id
                                                     ;

  // Scale the box and recenter it to the camera.
  // [ we do not need to scale as we clamp depth in the vertex shader to assure
  // maximum scaling, that's just show the alternative. ]
  glm::mat4 view = camera.view();
  view[3] = glm::vec4(glm::vec3(0.0f), view[3].w);
  auto const mvp = camera.proj() * glm::scale( view, glm::vec3(camera.far()));
  gx::SetUniform( pgm, "uMVP", mvp);

  uint32_t tex_id = sky_map_->id;
  
  // Debug visualization.
  if (mode == RenderMode::Sky) {
    if (kVisualizeIrradianceMap && irradiance_map_) {
      tex_id = irradiance_map_->id;
    } else if (kVisualizeSpecularMap && specular_map_) {
      tex_id = specular_map_->id;
    }
  }

  constexpr int32_t texture_unit = 0;
  gx::BindTexture( tex_id, texture_unit, gx::LinearMipmapClamp);
  gx::SetUniform( pgm, "uCubemap", texture_unit);

  gx::UseProgram(pgm);
    cube_mesh_->draw();
  gx::UseProgram(0u);

  gx::UnbindTexture();
  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
