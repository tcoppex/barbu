#include "fx/fbo.h"

#include "glm/gtc/type_ptr.hpp"     // for glm::value_ptr

#include "memory/assets/assets.h"   // for TEXTURES_ASSETS [tmp]
#include "ui/imgui_wrapper.h"

/* -------------------------------------------------------------------------- */

namespace {

/* [TODO] Return true when the internal format match the attachment. */
static
bool CheckInternalFormatMatchAttachment(int32_t _internalFormat, GLenum _attachment) {
  switch (_attachment) {
    case GL_DEPTH:
      switch (_internalFormat) {
        default:
          return true;
      }
    break;

    case GL_STENCIL:
      switch (_internalFormat) {
        default:
          return true;
      }
    break;

    case GL_DEPTH_STENCIL:
      switch (_internalFormat) {
        default:
          return true;
      }    
    break;

    default:
      return true;
  };
  return true;
}

}

/* -------------------------------------------------------------------------- */

bool Fbo::setup(int32_t _width, int32_t _height, int32_t _internalFormat) {
  assert((_width > 0) && (_width <= gx::GetI(GL_MAX_FRAMEBUFFER_WIDTH)));
  assert((_height > 0) && (_height <= gx::GetI(GL_MAX_FRAMEBUFFER_HEIGHT)));

  width_  = _width;
  height_ = _height;

  // Create the Framebuffer if it does not exists.
  if (!isInitialized()) {
    glCreateFramebuffers(1, &fbo_);
    if (!isInitialized()) { 
      return false; 
    }
  }
  textures_.clear();
  attachments_.clear();

  // Attach a renderbuffer when needed.
  if (kUseDepthBuffer) {
    addRenderbufferAttachment(kDefaultDepthFormat, GL_DEPTH_ATTACHMENT);
  }

  // Create a default color attachment.
  addColorAttachment(_internalFormat);

  // ---------------------------
  // glNamedFramebufferDrawBuffers(fbo_, attachments_.size(), attachments_.data());
  // ---------------------------

  return checkStatus();
}

void Fbo::release() {
  textures_.clear();

  if (isInitialized()) {
    glDeleteFramebuffers(1, &fbo_);
    fbo_ = 0u;
  }

  if (hasRenderbuffer()) {
    glDeleteRenderbuffers(1, &renderbuffer_);
    renderbuffer_ = 0u;
  }
}

bool Fbo::checkStatus() const noexcept {
  return GL_FRAMEBUFFER_COMPLETE == glCheckNamedFramebufferStatus(fbo_, GL_FRAMEBUFFER);
}

void Fbo::begin() noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  // gx::Viewport( width_, height_);
}

void Fbo::end() noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, 0u);
  // gx::Viewport( old_viewport_width_, old_viewport_height_);
  CHECK_GX_ERROR();
}

TextureHandle Fbo::addColorAttachment(int32_t _internalFormat, int32_t _attachment_index) noexcept {
  return addAttachment(_internalFormat, GL_COLOR_ATTACHMENT0 + _attachment_index);
}

TextureHandle Fbo::addDepthAttachment(int32_t _internalFormat) noexcept {
  return addAttachment(_internalFormat, GL_DEPTH_ATTACHMENT);
}

TextureHandle Fbo::addStencilAttachment(int32_t _internalFormat) noexcept {
  return addAttachment(_internalFormat, GL_STENCIL_ATTACHMENT);
}

TextureHandle Fbo::addDepthStencilAttachment(int32_t _internalFormat) noexcept {
  return addAttachment(_internalFormat, GL_DEPTH_STENCIL_ATTACHMENT);
}

void Fbo::addRenderbufferAttachment(int32_t _internalFormat, GLenum _attachment) noexcept {
  assert(isInitialized());
  assert(CheckInternalFormatMatchAttachment(_internalFormat, _attachment));

  // Allocate the Renderbuffer if needed.
  if (!hasRenderbuffer()) {
    glCreateRenderbuffers(1, &renderbuffer_);
  }

  // Create the depth buffer attachement.
  glNamedRenderbufferStorageMultisample(renderbuffer_, kDefaultMSAANumSamples, _internalFormat, width_, height_);
  
  glNamedFramebufferRenderbuffer(fbo_, _attachment, GL_RENDERBUFFER, renderbuffer_);
}

void Fbo::clearColorBuffer(glm::vec4 const& _color, int32_t _attachment_index) const noexcept {
  assert(isInitialized());
  // assert(static_cast<size_t>(_id) < textures_.size()); //

  // Will fails if multisampling is incorrectly set.
  glClearNamedFramebufferfv(fbo_, GL_COLOR, _attachment_index, glm::value_ptr(_color));
  CHECK_GX_ERROR();
}

void Fbo::clearDepthBuffer(float _depth) const noexcept {
  assert(isInitialized());
  glClearNamedFramebufferfv(fbo_, GL_DEPTH, 0, &_depth);
}

void Fbo::clearStencilBuffer(int32_t _stencil) const noexcept {
  assert(isInitialized());
  glClearNamedFramebufferiv(fbo_, GL_STENCIL, 0, &_stencil);
}

void Fbo::clearDepthStencilBuffer(float _depth, int32_t _stencil) const noexcept {
  assert(isInitialized());
  glClearNamedFramebufferfi(fbo_, GL_DEPTH_STENCIL, 0, _depth, _stencil);
}

void Fbo::debugDraw(std::string_view const& _name) const noexcept {
  std::string const window_id{ 
    _name.empty() ? std::string("FBO textures ") + std::to_string((uintptr_t)this)
                  : _name
  };

  ImGui::Begin( window_id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    auto const render_tex = [](GLuint tex) {
      auto const s = 320;
      auto id = (void*)(intptr_t)(tex);
      ImGui::Image( id, ImVec2(s, 3*s/4), ImVec2(0, 0), ImVec2(1,-1));
    };
    for (auto &tex : textures_) {
      render_tex(tex->id);
    }
  }
  ImGui::End();
}

/* -------------------------------------------------------------------------- */

TextureHandle Fbo::addAttachment(int32_t _internalFormat, GLenum _attachment) noexcept {
  assert(isInitialized());
  assert(CheckInternalFormatMatchAttachment(_internalFormat, _attachment));

  // Create the internal texture.
  auto tex = TEXTURE_ASSETS.create2d( 
    TEXTURE_ASSETS.findUniqueID(kDefaultFboTextureName), 
    kDefaultTextureLevels, 
    _internalFormat, 
    width_, 
    height_
  );
  CHECK_GX_ERROR();

  // Attach texture to the framebuffer.
  if (tex) {
    textures_.push_back(tex);
    attachments_.push_back(_attachment);

    glNamedFramebufferTexture(fbo_, _attachment, tex->id, 0);
  }
  CHECK_GX_ERROR();

  return tex;
}

/* -------------------------------------------------------------------------- */
