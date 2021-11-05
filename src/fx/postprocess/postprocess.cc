#include "fx/postprocess/postprocess.h"

#include "core/camera.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/imgui_wrapper.h"

#include "shaders/postprocess/linear_depth/interop.h"

// ----------------------------------------------------------------------------

void Postprocess::init() {
  // VAO.
  glCreateVertexArrays(1, &vao_);
  
  // Shader.
  pgm_.mapscreen = PROGRAM_ASSETS.createRender(
    "pp::composition",
    SHADERS_DIR "/postprocess/vs_mapscreen.glsl",
    SHADERS_DIR "/postprocess/fs_composition.glsl"
  );

  pgm_.lindepth = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/postprocess/linear_depth/cs_lindepth.glsl"
  );

  ssao_.init();

  CHECK_GX_ERROR();
}

void Postprocess::setup_textures(Camera const& camera) {
  auto const w = camera.width();
  auto const h = camera.height();
  bool const bResized = (w_ != w) || (h_ != h);

  if (bResized) {
    if (bTextureInit_) {
      release_textures();
    }

    w_ = w;
    h_ = h;
    create_textures();
  }
}

void Postprocess::create_textures() {
  // Initialize the FBOs with their internal buffer textures.
  glCreateFramebuffers(kNumBuffers, fbos_);
  
  for (int i = 0; i < kNumBuffers; ++i) {
    auto &buffer = buffers_[i];
    glCreateTextures(GL_TEXTURE_2D, kNumBufferTextureName, buffer.data());

    auto &fbo = fbos_[i];
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, buffer[COLOR_RGBA8], 0);
    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, buffer[DEPTH], 0);

    if (!gx::CheckFramebufferStatus()) {
      LOG_WARNING( "Postprocess framebuffer", i, "completion failed" );
    }
  }

  for (auto &buffer : buffers_) {
    for (int j = 0; j < kNumBufferTextureName; ++j) {
      glTextureStorage2D( buffer[j], 1, kBufferTextureFormats[j], w_, h_);
    }
  }
  current_buffer_ = 0;

  // sub-effects.
  {
    // LinearDepth.
    glCreateTextures(GL_TEXTURE_2D, 1, &out_.lindepth_r32f);

    // (we might want to scaled down the texture for post-effects)
    lindepth_res_ = glm::vec2( w_, h_);
    glTextureStorage2D(out_.lindepth_r32f, 1, kLinearDepthFormat, lindepth_res_.x, lindepth_res_.y);  
  
    ssao_.create_textures(w_, h_);
  }

  bTextureInit_ = true;

  CHECK_GX_ERROR();
}

void Postprocess::release_textures() {
  if (!bTextureInit_) {
    return;
  }

  glDeleteFramebuffers(kNumBuffers, fbos_);
  for (int i = 0; i < kNumBuffers; ++i) {
    glDeleteTextures( kNumBufferTextureName, buffers_[i].data());
  }
  glDeleteTextures(1, &out_.lindepth_r32f);
  out_.lindepth_r32f = 0u;
  
  ssao_.release_textures();
  bTextureInit_ = false;

  CHECK_GX_ERROR();
}

void Postprocess::deinit() {
  release_textures();
  glDeleteVertexArrays(1, &vao_);
  //ssao_.deinit();

  CHECK_GX_ERROR();
}

void Postprocess::begin() {
  assert(bTextureInit_);

  if (!bEnable_) {
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbos_[current_buffer_]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  CHECK_GX_ERROR();
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

void Postprocess::post_effects(Camera const& camera) {
  auto const pgm = pgm_.lindepth->id;
  int image_unit = -1;

  // Linearize Depth.
  gx::UseProgram(pgm);
  {
    gx::SetUniform( pgm, "uResolution", lindepth_res_);
    gx::SetUniform( pgm, "uLinearParams", camera.linearizationParams());

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
  auto const& pgm = pgm_.mapscreen->id;

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
