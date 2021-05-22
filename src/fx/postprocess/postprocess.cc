#include "fx/postprocess/postprocess.h"

#include "core/camera.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/imgui_wrapper.h"

#include "shaders/postprocess/linear_depth/interop.h"

// ----------------------------------------------------------------------------

void Postprocess::init(Camera const& camera) {
  auto const w = camera.width();
  auto const h = camera.height();

  // Initialize the FBOs with their internal buffer textures.
  glCreateFramebuffers(kNumBuffers, fbos_);
  for (int i = 0; i < kNumBuffers; ++i) {
    auto &buffer = buffers_[i];
    glCreateTextures(GL_TEXTURE_2D, kNumBufferTextureName, buffer.data());
    for (int j = 0; j < kNumBufferTextureName; ++j) {
      glTextureStorage2D( buffer[j], 1, kBufferTextureFormats[j], w, h);
    }

    auto &fbo = fbos_[i];
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, buffer[COLOR_RGBA8], 0);
    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, buffer[DEPTH], 0);
    
    if (!gx::CheckFramebufferStatus()) {
      LOG_WARNING( "Postprocess framebuffer", i, "completion failed" );
    }
  }
  current_buffer_ = 0;

  // VAO.
  glCreateVertexArrays(1, &vao_);
  
  // Shader.
  pgm_.mapscreen = PROGRAM_ASSETS.createRender(
    "pp::composition",
    SHADERS_DIR "/postprocess/vs_mapscreen.glsl",
    SHADERS_DIR "/postprocess/fs_composition.glsl"
  )->id;

  // sub-effects.
  init_post_effects(camera);

  CHECK_GX_ERROR();
}

void Postprocess::deinit() {
  glDeleteFramebuffers(kNumBuffers, fbos_);
  for (int i = 0; i < kNumBuffers; ++i) {
    glDeleteTextures( kNumBufferTextureName, buffers_[i].data());
  }
  glDeleteVertexArrays(1, &vao_);
}

void Postprocess::begin() {
  if (!bEnable_) {
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbos_[current_buffer_]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Postprocess::end(Camera const& camera) {
  if (!bEnable_) {
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

  // Filters input buffers.
  if (bEnable_) {
    post_effects(camera);
  } else {
    // clear ao buffer (todo ?)
    //glClearNamedFramebufferfv();
  }

  // Mapscreen for final composition.
  render_screen();

  // Copy the depth buffer to the main one to continue forward rendering.
  // (we might as well do this in the CS render_screen pass)
  auto const w = camera.width();
  auto const h = camera.height();
  glBlitNamedFramebuffer(
    fbos_[current_buffer_], 0u, 
    0, 0, w, h, 
    0, 0, w, h, 
    GL_DEPTH_BUFFER_BIT, 
    GL_NEAREST
  );

  // Swap internal buffers.
  current_buffer_ = (current_buffer_ + 1) % kNumBuffers;

  CHECK_GX_ERROR();
}

// ----------------------------------------------------------------------------

void Postprocess::init_post_effects(Camera const& camera) {
  // LinearDepth.
  glCreateTextures(GL_TEXTURE_2D, 1, &out_.lindepth_r32f);

  // (we might want to scaled down the texture for post-effects)
  lindepth_res_ = glm::vec2(camera.width(), camera.height());
  glTextureStorage2D(out_.lindepth_r32f, 1, kLinearDepthFormat, lindepth_res_.x, lindepth_res_.y);

  pgm_.lindepth = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/postprocess/linear_depth/cs_lindepth.glsl"
  )->id;

  ssao_.init(camera);

  CHECK_GX_ERROR();
}

void Postprocess::post_effects(Camera const& camera) {
  int image_unit = -1;

  // Linearize Depth.
  gx::UseProgram(pgm_.lindepth);
  {
    auto &pgm = pgm_.lindepth;
    
    gx::SetUniform( pgm, "uResolution", lindepth_res_);
    gx::SetUniform( pgm, "uLinearParams", camera.linearization_params());

    gx::BindTexture(buffer_texture(DEPTH), ++image_unit);
    gx::SetUniform( pgm, "uDepthIn", image_unit);

    glBindImageTexture( ++image_unit, out_.lindepth_r32f, 0, GL_FALSE, 0, GL_WRITE_ONLY, kLinearDepthFormat); //
    gx::SetUniform( pgm, "uLinearDepthOut", image_unit);

    gx::DispatchCompute<LINEARDEPTH_BLOCK_DIM, LINEARDEPTH_BLOCK_DIM>( lindepth_res_.x, lindepth_res_.y);
  } 
  gx::UseProgram();

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // Unbind all.
  for (int i=0; i<image_unit; ++i) {
    gx::UnbindTexture(i);
    glBindImageTexture( i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
  }

  // SSAO.
  ssao_.process(camera, out_.lindepth_r32f, out_.ext_ao_r32f); 

  CHECK_GX_ERROR();
}

void Postprocess::render_screen() {
  auto const& pgm = pgm_.mapscreen;

  gx::Disable(gx::State::DepthTest);

  gx::UseProgram(pgm);
  {
    int image_unit = -1;
    
    gx::BindTexture( buffer_texture(COLOR_RGBA8), ++image_unit, gx::NearestClamp);
    gx::SetUniform(pgm, "uColor", image_unit);

    if (out_.ext_ao_r32f > 0) {
      gx::BindTexture( out_.ext_ao_r32f, ++image_unit, gx::LinearClamp);
      gx::SetUniform(pgm, "uAO", image_unit);
    }

    // [ wrap inside gx ]
    {
      glBindVertexArray(vao_);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);
    }

    for (int i = 0; i < image_unit; ++i) {
      gx::UnbindTexture(i);
    }
  }
  gx::UseProgram();

  gx::Enable(gx::State::DepthTest);

  // ------------

  #if 0
  ImGui::Begin("pp textures", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    auto const render_tex = [](GLuint tex) {
      auto const s = 320;
      auto const id = reinterpret_cast<ImTextureID>(tex);
      ImGui::Image( id, ImVec2(s, 3*s/4), ImVec2(0, 0), ImVec2(1,-1));
    };

    for (auto &tex : ssao_.textures_) {
      render_tex(tex);
    }
  }
  ImGui::End();
  #endif
}

// ----------------------------------------------------------------------------
