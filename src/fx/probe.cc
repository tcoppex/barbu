#include "fx/probe.h"

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
EnumArray<glm::mat4, CubeFace> const Probe::ViewController::kViewMatrices{
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

// Shared probe camera [does not handle concurrency].
Camera Probe::sCamera;

// ----------------------------------------------------------------------------

void Probe::init(int32_t const resolution) {
  resolution_ = resolution;

  // Initialize the shared camera if needed.
  if (sCamera.fov() <= 0.0f) {
    sCamera.set_perspective(90.0f, 1, 1, 0.1f, 500.0f); //
  }

  glCreateFramebuffers( 1, &fbo_);

  // Attach a renderbuffer for depth testing.
  glCreateRenderbuffers( 1, &renderbuffer_);
  glNamedRenderbufferStorage( renderbuffer_, GL_DEPTH_COMPONENT24, resolution_, resolution_);
  glNamedFramebufferRenderbuffer( fbo_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_);

  // Create a HDR cubemap texture.
  texture_ = TEXTURE_ASSETS.createCubemap( 
    kDefaultProbeName, 1, kProbeInternalFormat, resolution_, resolution_
  );
  assert( texture_ != nullptr );
  glNamedFramebufferTexture(fbo_, GL_COLOR_ATTACHMENT0, texture_->id, 0);
  
  if (!gx::CheckFramebufferStatus()) {
    LOG_ERROR( "Probe framebuffer completion has failed." );
  }

  CHECK_GX_ERROR();
}

void Probe::release() {
  if (fbo_ != 0u) {
    glDeleteFramebuffers( 1, &fbo_);
    fbo_ = 0u;

    glDeleteRenderbuffers( 1, &renderbuffer_);
    renderbuffer_ = 0u;
  }

  // [todo : release texture asset]
}

void Probe::begin() {
  sCamera.set_controller(&view_controller_);
  gx::Viewport(resolution_, resolution_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Probe::end() {
  sCamera.set_controller(nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, 0u);
}

Camera const& Probe::setup_face(CubeFace face) {
  auto const target{ GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int32_t>(face) };
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texture_->id, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //
  
  // Update camera internal matrices.
  view_controller_.set_face(face);
  sCamera.rebuild();

  return sCamera;
}

void Probe::process(DrawCallback_t draw_cb) {
  begin();
  for (auto const& face : kIterFaces) {
    setup_face(face);
    draw_cb(sCamera);
  }
  end();
}

// ----------------------------------------------------------------------------