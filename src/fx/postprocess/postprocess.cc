#include "fx/postprocess/postprocess.h"

#include "core/camera.h"
#include "core/logger.h"
#include "memory/assets/assets.h"
#include "ui/imgui_wrapper.h"

#include "shaders/postprocess/linear_depth/interop.h"

// ----------------------------------------------------------------------------

void Postprocess::init() {
  // Linearize Depth program.
  lindepth_.pgm = PROGRAM_ASSETS.createCompute( 
    SHADERS_DIR "/postprocess/linear_depth/cs_lindepth.glsl"
  );

  // SSAO subeffect.
  ssao_.init();

  // RGBA8 Blur.
  {
    blur_rgba8_.pgm = PROGRAM_ASSETS.createCompute( 
      SHADERS_DIR "/postprocess/cs_blur.glsl"
    );
  }

  // ScreenMapping.
  {
    glCreateVertexArrays(1, &mapscreen_.vao);
    
    mapscreen_.pgm = PROGRAM_ASSETS.createRender(
      "Postprocess::Composition",
      SHADERS_DIR "/postprocess/vs_mapscreen.glsl",
      SHADERS_DIR "/postprocess/fs_composition.glsl"
    );
  }

  CHECK_GX_ERROR();
}

void Postprocess::setupTextures(Camera const& camera) {
  auto const w = camera.width();
  auto const h = camera.height();
  auto const has_resized = (width_ != w) || (height_ != h);

  if (has_resized) {
    width_  = w;
    height_ = h;

    // Framebuffers.
    for (auto& fbo : FBOs_) {
      fbo.setup(width_, height_, kBufferTextureFormats[COLOR_RGBA8]);
      fbo.addColorAttachment(kBufferTextureFormats[EXTRA_RGBA8]); //
      fbo.addDepthAttachment(kBufferTextureFormats[DEPTH]);
    }
    current_buffer_ = 0;

    // In-between textures.
    releaseTextures();
    createTextures();
  }
}

void Postprocess::createTextures() {
  glm::vec2 const resolution( width_, height_);

  // LinearDepth.
  {
    lindepth_.resolution = 1.0f * resolution; //
    int32_t const lvl = 1;
    int32_t const w = static_cast<int32_t>(lindepth_.resolution.x);
    int32_t const h = static_cast<int32_t>(lindepth_.resolution.y);

    // Bug on resize ?
    // (This might fails to recreate a new texture, and send the last one instead)
    lindepth_.tex = TEXTURE_ASSETS.create2d( 
      "PostProcess::linearizeDepth", lvl, kLinearDepthFormat, w, h
    );
  }

  // Screen-Space Ambient Occlusion.
  ssao_.createTextures(width_, height_);

  // Blur RGBA8.
  {
    blur_rgba8_.resolution = 1.0f * resolution; //
    int32_t const lvl = 1;
    int32_t const w = static_cast<int32_t>(blur_rgba8_.resolution.x);
    int32_t const h = static_cast<int32_t>(blur_rgba8_.resolution.y);
    blur_rgba8_.tex = TEXTURE_ASSETS.create2d( 
      "PostProcess::BlurEmissive", lvl, GL_RGBA8, w, h //
    );
  }

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
  glDeleteVertexArrays(1, &mapscreen_.vao);
  releaseTextures();
  for (auto& fbo : FBOs_) {
    fbo.release();
  }
}

void Postprocess::begin() {
  assert(bTextureInit_);
  if (!bEnable_) {
    return;
  }

  currentFBO().begin();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //
  // currentFBO().clearColorBuffer(glm::vec4(0.0f), COLOR_RGBA8);
  // currentFBO().clearDepthBuffer(1.0);
  
  // The Extra buffer is currently used entirely by the EmissiveMap, which filters
  // its postprocess stage wrt its alpha channel.
  currentFBO().clearColorBuffer(glm::vec4(0.0f), EXTRA_RGBA8);
}

void Postprocess::end(Camera const& camera) {
  if (!bEnable_) {
    return;
  }

  auto &fbo{ currentFBO() };
  fbo.end();

  // Synchronizes framebuffer to be sure operation has finished.
  glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT); //

  // Apply post-processing filters.
  applyEffects(camera);

  // Mapscreen for final composition.
  renderScreen();

  // Copy the depth buffer to the main one to continue forward rendering.
  // (we might as well do this in the CS renderScreen pass)
  fbo.draw(0, 0, camera.width(), camera.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST); //

  // Swap internal buffers.
  current_buffer_ = (current_buffer_ + 1) % kNumBuffers;

  CHECK_GX_ERROR();
}

GLuint Postprocess::bufferTextureID(BufferTextureName_t const name) const noexcept {
  GLuint attachment(0);

  switch (name) {
    case COLOR_RGBA8:
      attachment = GL_COLOR_ATTACHMENT0;
    break;

    case EXTRA_RGBA8:
      attachment = GL_COLOR_ATTACHMENT1;
    break;

    case DEPTH:
      attachment = GL_DEPTH_ATTACHMENT;
    break;

    default:
      LOG_ERROR("Buffer name not recognized.");
  }

  return currentFBO().texture(attachment)->id;
}

// ----------------------------------------------------------------------------

void Postprocess::applyEffects(Camera const& camera) {

  // Linearize Depth.
  {
    auto const pgm = lindepth_.pgm->id;
    auto const tex = lindepth_.tex->id;
    int image_unit = -1;
    
    gx::UseProgram(pgm);
    {
      // Params.
      gx::SetUniform( pgm, "uResolution", lindepth_.resolution);
      gx::SetUniform( pgm, "uLinearParams", camera.linearizationParams());

      // Input.
      gx::BindTexture( bufferTextureID(DEPTH), ++image_unit);
      gx::SetUniform( pgm, "uDepthIn", image_unit);

      // Output.
      glBindImageTexture( ++image_unit, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, kLinearDepthFormat); //
      gx::SetUniform( pgm, "uLinearDepthOut", image_unit);

      auto const width  = static_cast<uint32_t>(lindepth_.resolution.x);
      auto const height = static_cast<uint32_t>(lindepth_.resolution.y);
      gx::DispatchCompute<LINEARDEPTH_BLOCK_DIM, LINEARDEPTH_BLOCK_DIM>( width, height);
    } 
    gx::UseProgram();

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind all.
    for (int i = 0; i < image_unit; ++i) {
      gx::UnbindTexture(i);
      glBindImageTexture( i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    }

    CHECK_GX_ERROR();
  }

  // SSAO.
  ssao_.applyEffect(camera, lindepth_.tex->id, output_texid_.ao); 

  // Blur Emissive RGBA.
  // TODO : Downsample the emissive map.
#if 1
  {
    auto const pgm = blur_rgba8_.pgm->id; //
    auto const tex = blur_rgba8_.tex->id;
    int image_unit = -1;
    
    gx::UseProgram(pgm);
    {
      // Params.
      gx::SetUniform(pgm, "uRadius", 8);
      
      // Input.
      gx::BindTexture(bufferTextureID(EXTRA_RGBA8), ++image_unit, gx::NearestClamp);
      gx::SetUniform(pgm, "uSrcTex", image_unit);

      // Output.
      glBindImageTexture(++image_unit, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8); //
      gx::SetUniform(pgm, "uDstImg", image_unit);

      auto const width  = static_cast<uint32_t>(blur_rgba8_.resolution.x);
      auto const height = static_cast<uint32_t>(blur_rgba8_.resolution.y);

      const int TILE_SIZE = 32; //
      gx::DispatchCompute<TILE_SIZE, TILE_SIZE>(width, height); //
    } 
    gx::UseProgram();

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind all.
    for (int i = 0; i < image_unit; ++i) {
      gx::UnbindTexture(i);
      glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    }

    CHECK_GX_ERROR();
  }
#endif

  CHECK_GX_ERROR();
}

void Postprocess::renderScreen() {
  auto const& pgm = mapscreen_.pgm->id;

  gx::Disable(gx::State::DepthTest); //
  gx::UseProgram(pgm);
  {
    int image_unit = -1;
    
    // Albedo.
    gx::BindTexture( bufferTextureID(COLOR_RGBA8), ++image_unit, gx::NearestClamp);
    gx::SetUniform( pgm, "uAlbedo", image_unit);

    // Emissive.
    gx::BindTexture( blur_rgba8_.tex->id, ++image_unit, gx::NearestClamp);
    gx::SetUniform( pgm, "uEmissive", image_unit);

    // AO.
    if (output_texid_.ao > 0) {
      gx::BindTexture( output_texid_.ao, ++image_unit, gx::LinearClamp);
      gx::SetUniform( pgm, "uAO", image_unit);
    }

    // Render screen-quad.
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
  debugDraw("[debug] PostProcess view");
  // currentFBO().debugDraw("Raw postprocess FBO textures");
#endif
}

// ----------------------------------------------------------------------------

void Postprocess::debugDraw(std::string_view const& _label) const noexcept {
  ImGui::Begin(_label.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    auto const render_tex = [](auto tex) {
      float const width  = 320.0f; 
      float const height = width / tex->ratio();
      imgui_utils::display_texture(tex->id, width, height);
    };

    render_tex(blur_rgba8_.tex);
  }
  ImGui::End();
}

// ----------------------------------------------------------------------------
