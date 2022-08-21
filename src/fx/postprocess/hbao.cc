#include "hbao.h"
#include "shaders/postprocess/ssao/interop.h"

#include "core/camera.h"
#include "memory/assets/assets.h"
#include "ui/imgui_wrapper.h"

// ----------------------------------------------------------------------------

static constexpr bool kShowUI = true;

// ----------------------------------------------------------------------------

const int HBAO::kHBAOTileWidth = HBAO_TILE_WIDTH;
const int HBAO::kBlurRadius    = HBAO_BLUR_RADIUS;

// ----------------------------------------------------------------------------

namespace {

static void bindImageTexture(GLint unit, GLuint tex, GLenum mode, GLenum format) {
  glBindImageTexture( unit, tex, 0, GL_FALSE, 0, mode, format); //
}

} // namespace

// ----------------------------------------------------------------------------

void HBAO::init() {
  pgm_.ssao    = PROGRAM_ASSETS.createCompute( SHADERS_DIR "/postprocess/ssao/cs_hbao.glsl" );
  pgm_.blur_x  = PROGRAM_ASSETS.createCompute( SHADERS_DIR "/postprocess/ssao/cs_blur_ao_x.glsl" );
  pgm_.blur_y  = PROGRAM_ASSETS.createCompute( SHADERS_DIR "/postprocess/ssao/cs_blur_ao_y.glsl" );

  // check we found the subroutine location.
  LOG_CHECK( glGetSubroutineUniformLocation(pgm_.ssao->id, GL_COMPUTE_SHADER, "suHBAO") > -1 ); //

  CHECK_GX_ERROR();
}

void HBAO::createTextures(int32_t w, int32_t h, float const scaling) {
  glm::vec2 const res{ static_cast<float>(w), static_cast<float>(h) };
  params_.full_resolution = glm::vec4( res, 1.0f / res);
  params_.ao_resolution   = glm::vec4( scaling * res, 1.0f / (scaling * res));

  GLsizei const width  = static_cast<GLsizei>(params_.ao_resolution.x); 
  GLsizei const height = static_cast<GLsizei>(params_.ao_resolution.y);

  // Textures.
  glCreateTextures( GL_TEXTURE_2D, kNumTextureName, textures_.data());
  for (int i = 0; i < kNumTextureName; ++i) {
    glTextureStorage2D(textures_[i], 1, kTextureFormats[i], width, height);
  }

  CHECK_GX_ERROR();
}

void HBAO::releaseTextures() {
  glDeleteTextures(((GLsizei)textures_.size()), textures_.data()); //
  CHECK_GX_ERROR();
}

void HBAO::applyEffect(Camera const& camera, GLuint const tex_linear_depth, GLuint &tex_ao_out) {
  assert(0 != tex_linear_depth);

  updateParameters(camera);

  computeHBAO(tex_linear_depth);
  computeBlurAO();

  tex_ao_out = textures_[BLUR_AO_XY];

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void HBAO::updateParameters(Camera const& camera) {
  auto constexpr inv_ln_two     = 1.44269504f;
  auto constexpr sqrt_ln_two    = 0.832554611f;
  
  auto const z_near             = camera.znear();
  auto const z_far              = camera.zfar();
  auto const fov_y              = camera.fov();
  auto const radius_scaled      = std::max(ui_params_.radius, glm::epsilon<float>());
  auto const scene_scale        = std::min(z_near, z_far);
  auto const blur_sigma         = (ui_params_.blur_radius + 1.0f) * 0.5f;
  auto const fl                 = glm::vec2( params_.ao_resolution.y / params_.ao_resolution.x, 1.0) / tanf(0.5f * fov_y);
  auto const inv_fl             = 1.0f / fl;

  auto &P = params_;
  P.focal_length                = glm::vec4(fl, inv_fl);
  P.uv_to_view                  = glm::vec4(2.0f, -2.0f, -1.0f, 1.0f) * glm::vec4(inv_fl, inv_fl);
  P.radius_squared              = glm::pow(radius_scaled * scene_scale, 2.0f);
  P.tan_angle_bias              = tanf(ui_params_.angle_bias);
  P.strength                    = 1.0f;
  P.blur_depth_threshold        = 2.0f * sqrt_ln_two * (scene_scale / ui_params_.blur_sharpness);
  P.blur_falloff                = inv_ln_two / (2.0f * blur_sigma * blur_sigma);

  // Debug UI.
  if constexpr(kShowUI) {
    ImGui::Begin("HBAO", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::DragFloat("radius",         &ui_params_.radius,         0.0005f, 0.001f, 0.5f);
      ImGui::DragFloat("Blur radius",    &ui_params_.blur_radius,    0.05f, 0.01f, 16.0f);
      ImGui::DragFloat("Blur sharpness", &ui_params_.blur_sharpness, 0.1f, 0.01f, 64.0f);
      ImGui::DragFloat("Angle bias",     &ui_params_.angle_bias,     0.01f, 0.01f, 1.14f);

      // Display textures.
      float const width = 320.0f;
      float const height = params_.full_resolution.y * width / params_.full_resolution.x;
      for (auto &tex_id : textures_) {
        imgui_utils::display_texture(tex_id, width, height);
      }
    ImGui::End();
  }
}

void HBAO::computeHBAO(GLuint const tex_linear_depth) {
  auto const pgm = pgm_.ssao->id; 

  uint32_t const width  = static_cast<uint32_t>(params_.ao_resolution.x); //
  uint32_t const height = static_cast<uint32_t>(params_.ao_resolution.y); //

  gx::UseProgram(pgm);
  {
    int image_unit = -1;

    // Parameters.
    gx::SetUniform( pgm, "uAOResolution",   params_.ao_resolution);
    gx::SetUniform( pgm, "uUVToView",       params_.uv_to_view);
    gx::SetUniform( pgm, "uR2",             params_.radius_squared);
    gx::SetUniform( pgm, "uTanAngleBias",   params_.tan_angle_bias);
    gx::SetUniform( pgm, "uStrength",       params_.strength);

    gx::BindTexture(tex_linear_depth, ++image_unit, gx::LinearClamp);
    gx::SetUniform(pgm, "uTexLinearDepth", image_unit);

    // First Pass, on X.
    {
      // Specify the subroutine.
      auto const ssao_x_id = glGetSubroutineIndex(pgm, GL_COMPUTE_SHADER, "HBAO_X");
      assert(GL_INVALID_INDEX != ssao_x_id);
      glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &ssao_x_id);

      // Output : X-HBAO.
      bindImageTexture( ++image_unit, textures_[AO_X], GL_WRITE_ONLY, kTextureFormats[AO_X]);
      gx::SetUniform( pgm, "uImgOutputX", image_unit);

      // Launch X-HBAO.
      gx::DispatchCompute<kHBAOTileWidth, 1u>(width, height);

      // Unbind output.
      glBindImageTexture( image_unit, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F); //
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // ---------------------------------------

    // Second Pass, on Y.
    {
      auto const ssao_y_id = glGetSubroutineIndex(pgm, GL_COMPUTE_SHADER, "HBAO_Y");
      assert(GL_INVALID_INDEX != ssao_y_id);
      glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &ssao_y_id);

      // Y-HBAO (2nd) input.
      // (switch from write-only to read-only)
      bindImageTexture( image_unit, textures_[AO_X], GL_READ_ONLY, kTextureFormats[AO_X]);
      gx::SetUniform( pgm, "uImgInputX", image_unit);

      // Y-HBAO output.
      bindImageTexture( ++image_unit, textures_[AO_XY], GL_WRITE_ONLY, kTextureFormats[AO_XY]);
      gx::SetUniform( pgm, "uImgOutputXY", image_unit);
      
      // Launch Y-HBAO.
      gx::DispatchCompute<kHBAOTileWidth, 1u>(height, width);
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind all.
    for (int i = 0; i < image_unit; ++i) {
      bindImageTexture( i, 0, GL_WRITE_ONLY, GL_R32F); //
    }
  }
  gx::UseProgram(0);

  CHECK_GX_ERROR();
}

void HBAO::computeBlurAO() {
  uint32_t const width  = static_cast<uint32_t>(params_.ao_resolution.x); //
  uint32_t const height = static_cast<uint32_t>(params_.ao_resolution.y); //

  /// Horizontal blur pass.
  gx::UseProgram(pgm_.blur_x->id);
  {
    auto &pgm = pgm_.blur_x->id;

    gx::SetUniform(pgm, "uBlurFalloff",         params_.blur_falloff);
    gx::SetUniform(pgm, "uBlurDepthThreshold",  params_.blur_depth_threshold);
    gx::SetUniform(pgm, "uResolution",          params_.ao_resolution); //

    // Input textures.
    gx::BindTexture(textures_[AO_XY], 0, gx::NearestClamp);
    gx::SetUniform(pgm, "uTexAONearest", 0);
    gx::BindTexture(textures_[AO_XY], 1, gx::LinearClamp);
    gx::SetUniform(pgm, "uTexAOLinear", 1);

    // Output image.
    bindImageTexture( 2, textures_[BLUR_AO_X], GL_WRITE_ONLY, kTextureFormats[BLUR_AO_X]);
    gx::SetUniform(pgm, "uDstImg", 2);

    gx::DispatchCompute<HBAO_BLUR_BLOCK_DIM, 1u>(width, height);
  }
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // ---------------------------------------

  /// Vertical blur pass
  gx::UseProgram(pgm_.blur_y->id);
  {
    auto &pgm = pgm_.blur_y->id;

    gx::SetUniform(pgm, "uBlurFalloff",         params_.blur_falloff);
    gx::SetUniform(pgm, "uBlurDepthThreshold",  params_.blur_depth_threshold);
    gx::SetUniform(pgm, "uResolution",          params_.ao_resolution);

    // Input textures.
    gx::BindTexture(textures_[BLUR_AO_X], 0, gx::NearestClamp);
    gx::SetUniform(pgm, "uTexAONearest", 0);
    
    gx::BindTexture(textures_[BLUR_AO_X], 1, gx::LinearClamp);
    gx::SetUniform(pgm, "uTexAOLinear", 1);

    // Output image.
    bindImageTexture( 2, textures_[BLUR_AO_XY], GL_WRITE_ONLY, kTextureFormats[BLUR_AO_XY]);
    gx::SetUniform(pgm, "uDstImg", 2);

    gx::DispatchCompute<HBAO_BLUR_BLOCK_DIM, 1u>(height, width);
  }
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  for (int i = 0; i < 3; ++i) {
    gx::UnbindTexture(i);
  }

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------
