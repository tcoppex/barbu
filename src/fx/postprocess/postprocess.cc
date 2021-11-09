#include "fx/postprocess/postprocess.h"

#include "core/camera.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/imgui_wrapper.h"

#include "shaders/postprocess/linear_depth/interop.h"

// ----------------------------------------------------------------------------

void Postprocess::init() {
  // MapScreen program.
  {
    glCreateVertexArrays(1, &mapscreen_.vao);
    
    mapscreen_.pgm = PROGRAM_ASSETS.createRender(
      "Postprocess::Composition",
      SHADERS_DIR "/postprocess/vs_mapscreen.glsl",
      SHADERS_DIR "/postprocess/fs_composition.glsl"
    );
  }

  // Linearize Dept program.
  lindepth_.pgm = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/postprocess/linear_depth/cs_lindepth.glsl"
  );

  // SSAO subeffect.
  ssao_.init();

  CHECK_GX_ERROR();
}

void Postprocess::setupTextures(Camera const& camera) {
  auto const w = camera.width();
  auto const h = camera.height();
  auto const has_resized = (width_ != w) || (height_ != h);

  if (has_resized) {
    width_  = w;
    height_ = h;

    releaseTextures();
    createTextures();

    for (auto& fbo : FBOs_) {
      // Note : this will create a useless render buffer everytime !!
      fbo.setup(width_, height_, kBufferTextureFormats[COLOR_RGBA8]);
      fbo.addDepthAttachment(kBufferTextureFormats[DEPTH]);
    }
    current_buffer_ = 0;
  }
}

void Postprocess::createTextures() {
  // LinearDepth.
  lindepth_.resolution = 1.0f * glm::vec2( width_, height_); //
  int32_t const w   = static_cast<int32_t>(lindepth_.resolution.x);
  int32_t const h   = static_cast<int32_t>(lindepth_.resolution.y);
  int32_t const lvl = 1;

  // Bug on resize ?
  // (This might fails to recreate a new texture and send the last one instead)
  lindepth_.tex = TEXTURE_ASSETS.create2d( 
    "PostProcess::linearizeDepth", lvl, kLinearDepthFormat, w, h
  );

  // Screen-Space Ambient Occlusion.
  ssao_.createTextures(width_, height_); //

  bTextureInit_ = true;
  CHECK_GX_ERROR();
}

void Postprocess::releaseTextures() {
  if (!bTextureInit_) {
    return;
  }
  bTextureInit_ = false;
  
  ssao_.releaseTextures();

  CHECK_GX_ERROR();
}

void Postprocess::deinit() {
  glDeleteVertexArrays(1, &mapscreen_.vao); //
  releaseTextures();
}

void Postprocess::begin() {
  assert(bTextureInit_);
  if (!bEnable_) {
    return;
  }

  FBOs_[current_buffer_].begin();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //
}

void Postprocess::end(Camera const& camera) {
  if (!bEnable_) {
    return;
  }

  auto &fbo{ FBOs_[current_buffer_] };

  fbo.end();
 
  glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

  // Filters input buffers.
  if constexpr(true) {
    applyEffects(camera);
  } else {
    // Clear ao buffer ?
    // fbo.clearDepthBuffer(0);
  }

  // Mapscreen for final composition.
  renderScreen();

  // Copy the depth buffer to the main one to continue forward rendering.
  // (we might as well do this in the CS renderScreen pass)
  auto const w = camera.width();
  auto const h = camera.height();
  auto const read_fbo  = fbo.id();
  auto const write_fbo = 0u;
  glBlitNamedFramebuffer(
    read_fbo, write_fbo, 
    0, 0, w, h, 
    0, 0, w, h, 
    GL_DEPTH_BUFFER_BIT, 
    GL_NEAREST
  );

  // Swap internal buffers.
  current_buffer_ = (current_buffer_ + 1) % kNumBuffers;

  CHECK_GX_ERROR();
}

GLuint Postprocess::bufferTextureID(BufferTextureName_t const name) const noexcept { 
  auto const attachment = (name == COLOR_RGBA8) ? GL_COLOR_ATTACHMENT0 
                                                : GL_DEPTH_ATTACHMENT
                                                ;
  return FBOs_[current_buffer_].texture(attachment)->id;
}

// ----------------------------------------------------------------------------

void Postprocess::applyEffects(Camera const& camera) {
  auto const pgm = lindepth_.pgm->id;
  auto const tex = lindepth_.tex->id;
  int image_unit = -1;

  // Linearize Depth.
  gx::UseProgram(pgm);
  {
    gx::SetUniform( pgm, "uResolution", lindepth_.resolution);
    gx::SetUniform( pgm, "uLinearParams", camera.linearizationParams());

    gx::BindTexture( bufferTextureID(DEPTH), ++image_unit);
    gx::SetUniform( pgm, "uDepthIn", image_unit);

    glBindImageTexture( ++image_unit, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, kLinearDepthFormat); //
    gx::SetUniform( pgm, "uLinearDepthOut", image_unit);

    uint32_t const width  = static_cast<uint32_t>(lindepth_.resolution.x); //
    uint32_t const height = static_cast<uint32_t>(lindepth_.resolution.y); //
    gx::DispatchCompute<LINEARDEPTH_BLOCK_DIM, LINEARDEPTH_BLOCK_DIM>( width, height);
  } 
  gx::UseProgram();

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // Unbind all.
  for (int i = 0; i < image_unit; ++i) {
    gx::UnbindTexture(i);
    glBindImageTexture( i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
  }

  // SSAO.
  ssao_.applyEffect(camera, tex, output_texid_.ao); 

  CHECK_GX_ERROR();
}

void Postprocess::renderScreen() {
  auto const& pgm = mapscreen_.pgm->id;

  gx::Disable(gx::State::DepthTest); //
  gx::UseProgram(pgm);
  {
    int image_unit = -1;
    
    gx::BindTexture( bufferTextureID(COLOR_RGBA8), ++image_unit, gx::NearestClamp);
    gx::SetUniform( pgm, "uColor", image_unit);

    if (output_texid_.ao > 0) {
      gx::BindTexture( output_texid_.ao, ++image_unit, gx::LinearClamp);
      gx::SetUniform( pgm, "uAO", image_unit);
    }

    // [ wrap inside gx ]
    {
      glBindVertexArray(mapscreen_.vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);
    }

    for (int i = 0; i < image_unit; ++i) {
      gx::UnbindTexture(i);
    }
  }
  gx::UseProgram();
  gx::Enable(gx::State::DepthTest); //

#ifndef NDEBUG
  debugDraw( "[debug] PostProcess view" );
#endif
}

void Postprocess::debugDraw(std::string_view const& _name) const noexcept {
  ImGui::Begin(_name.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    auto const render_tex = [](GLuint tex) {
      auto const s = 320;
      auto const id = (void*)(intptr_t)(tex);
      ImGui::Image( id, ImVec2(s, 3*s/4), ImVec2(0, 0), ImVec2(1,-1));
    };

    for (auto &tex : ssao_.textures_) {
      render_tex(tex);
    }
  }
  ImGui::End();
}

// ----------------------------------------------------------------------------
