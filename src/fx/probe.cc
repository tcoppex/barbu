#include "fx/probe.h"

#include <cstring>
#include "memory/assets/assets.h"

// ----------------------------------------------------------------------------

static constexpr char const* kDefaultProbeName{ "ProbeHDR" };
static constexpr uint32_t kProbeInternalFormat{ GL_RGBA16F }; //

// ----------------------------------------------------------------------------

// CubeFace iterration array.
EnumArray<CubeFace, CubeFace> const Probe::kIterFaces{
  CubeFace::PosX, CubeFace::NegX, 
  CubeFace::PosY, CubeFace::NegY, 
  CubeFace::PosZ, CubeFace::NegZ
};

// Probe::ViewController face view matrices.
// [this could be reduced to 3x3 matrices]
EnumArray<glm::mat4, CubeFace> const Probe::kViewMatrices{
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

// Shared probe camera.
Camera Probe::sCamera;

// ----------------------------------------------------------------------------

void Probe::setup(int32_t const resolution, int32_t const levels, bool bUseDepth) {
  resolution_ = resolution;
  levels_ = levels;

  // Initialize the shared camera if needed.
  if (!sCamera.initialized()) {
    LOG_DEBUG_INFO( "Setup the probe shared camera." );
    sCamera.setDefault();
  }

  if (fbo_ <= 0u) {
    glCreateFramebuffers( 1, &fbo_);
  }

  // Attach a renderbuffer for depth testing when needed.
  if (bUseDepth && (renderbuffer_ <= 0u)) {
    glCreateRenderbuffers( 1, &renderbuffer_);
    glNamedRenderbufferStorage( renderbuffer_, GL_DEPTH_COMPONENT24, resolution_, resolution_);
    glNamedFramebufferRenderbuffer( fbo_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_);
  }

  // Create a HDR cubemap texture.
  texture_ = TEXTURE_ASSETS.createCubemap( 
    TEXTURE_ASSETS.findUniqueID(kDefaultProbeName), 
    levels, 
    kProbeInternalFormat, 
    resolution_, resolution_
  );
  LOG_CHECK( texture_ != nullptr );

  CHECK_GX_ERROR();
}

void Probe::release() {
  if (fbo_ != 0u) {
    glDeleteFramebuffers( 1, &fbo_);
    fbo_ = 0u;
  }

  if (renderbuffer_ != 0u) {
    glDeleteRenderbuffers( 1, &renderbuffer_);
    renderbuffer_ = 0u;
  }
}

void Probe::capture(DrawCallback_t draw_cb) {
  begin();
  for (int32_t lvl=0; lvl<levels_; ++lvl) {
    for (auto const& face : kIterFaces) {
      setupFace(face, lvl);
      draw_cb(sCamera, lvl);
    }
  }
  end();
}

// ----------------------------------------------------------------------------

void Probe::begin() {
  sCamera.setController(&view_controller_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Probe::end() {
  sCamera.setController(nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, 0u);
  glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

  CHECK_GX_ERROR();
}

void Probe::setupFace(CubeFace face, int32_t level) {
  // Setup the viewport to appropriate level resolution.
  int32_t const kFaceResolution{ resolution_ / (1 << level) };
  gx::Viewport( kFaceResolution, kFaceResolution);

  // Attach the cubemap face to the framebuffer color output.
  auto const face_id{ static_cast<int32_t>(face) };
  glNamedFramebufferTextureLayer( fbo_, GL_COLOR_ATTACHMENT0, texture_->id, level, face_id);
  LOG_CHECK( gx::CheckFramebufferStatus() );

  // [ the user is responsible to clear the framebuffer when needed ]
  //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
  
  // Update camera internal matrices.
  view_controller_.setFace(face);
  sCamera.rebuild();
}

// ----------------------------------------------------------------------------
