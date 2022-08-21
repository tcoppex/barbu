#include "fx/fbo.h"

#include "glm/gtc/type_ptr.hpp"     // for glm::value_ptr

#include "memory/assets/assets.h"   // for TEXTURES_ASSETS [tmp]
#include "ui/imgui_wrapper.h"

/* -------------------------------------------------------------------------- */

namespace {

/* [TODO] Return true when the internal format match the attachment. */
static
bool CheckInternalFormatMatchAttachment(int32_t _internalFormat, GLenum _attachment) {
  // switch (_attachment) {
  //   case GL_DEPTH:
  //     switch (_internalFormat) {
  //       default:
  //         return true;
  //     }
  //   break;

  //   case GL_STENCIL:
  //     switch (_internalFormat) {
  //       default:
  //         return true;
  //     }
  //   break;

  //   case GL_DEPTH_STENCIL:
  //     switch (_internalFormat) {
  //       default:
  //         return true;
  //     }    
  //   break;

  //   default:
  //     return true;
  // };
  return true;
}

// static
// void Blit(Fbo const& read_fbo, Fbo& write_fbo, Rectangle src_rect, Rectangle dst_rect, GLbitfield mask, GLenum filter) {
//   GLuint const read_fbo  = read_fbo.id();
//   GLuint const write_fbo = write_fbo.id();
//   glBlitNamedFramebuffer(
//     read_fbo, 
//     write_fbo, 
//     src.x, src.y, src.z, src.w,
//     dst.x, dst.y, dst.z, dst.w,
//     mask, 
//     filter
//   );
// }

// static
// void Draw(Fbo const& read_fbo, Rectangle dst, GLbitfield mask, GLenum filter) {
//   GLuint const read_fbo  = read_fbo.id();
//   GLuint const write_fbo = 0u;
//   glBlitNamedFramebuffer(
//     read_fbo, 
//     write_fbo, 
//     0, 0, read_fbo.width(), read_fbo.height(),
//     dst.x, dst.y, dst.z, dst.w,
//     mask, 
//     filter
//   );
// }

}

/* -------------------------------------------------------------------------- */

bool Fbo::setup(int32_t _width, int32_t _height, int32_t _internalFormat) {
  assert((_width > 0) && (_width <= gx::GetI(GL_MAX_FRAMEBUFFER_WIDTH)));
  assert((_height > 0) && (_height <= gx::GetI(GL_MAX_FRAMEBUFFER_HEIGHT)));

  width_  = _width;
  height_ = _height;

  // Creates the Framebuffer if it does not exists.
  if (!isInitialized()) {
    glCreateFramebuffers(1, &fbo_);
    if (!isInitialized()) { 
      return false; 
    }
  }
  textures_.clear();
  attachments_.clear();
  colorAttachments_.clear();

  // Attaches a renderbuffer when needed.
  if (kUseDepthBuffer) {
    addRenderbufferAttachment(kDefaultDepthFormat, GL_DEPTH_ATTACHMENT);
  }

  // Creates a default color attachment.
  addColorAttachment(_internalFormat);

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

void Fbo::begin() const noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  
  // glNamedFramebufferDrawBuffers(fbo_, colorAttachments_.size(), colorAttachments_.data());
  glDrawBuffers(colorAttachments_.size(), colorAttachments_.data());

  // gx::Viewport( width_, height_);
}

void Fbo::end() const noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, 0u);
  // gx::Viewport( old_viewport_width_, old_viewport_height_);

  CHECK_GX_ERROR();
}

TextureHandle Fbo::addColorAttachment(int32_t _internalFormat, int32_t _attachment_index) noexcept {
  auto const attachment = GL_COLOR_ATTACHMENT0 + _attachment_index;
  auto tex = addAttachment(_internalFormat, attachment);

  // Automatically adds (all) color buffers to outputs.
  if (tex) {
    colorAttachments_.push_back(attachment);
  }

  return tex;
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

  // Allocates the Renderbuffer if needed.
  if (!hasRenderbuffer()) {
    glCreateRenderbuffers(1, &renderbuffer_);
  }

  // Creates the depth buffer attachement.
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

TextureHandle Fbo::texture(GLenum _attachment) const noexcept {
  for (size_t i(0); i < attachments_.size(); ++i) {
    if (attachments_[i] == _attachment) {
      return textures_[i];
    }
  }
  return nullptr;
}

TextureHandle Fbo::colorTexture(int32_t _attachment_index) const noexcept {
  return texture(GL_COLOR_ATTACHMENT0 + _attachment_index);
}

TextureHandle Fbo::depthTexture() const noexcept {
  if (auto tex = texture(GL_DEPTH_ATTACHMENT); tex) {
    return tex;
  }
  return texture(GL_DEPTH_STENCIL_ATTACHMENT);
}

void Fbo::draw(float _x, float _y, float _w, float _h, GLbitfield _mask, GLenum _filter) const noexcept {
  auto const read_fbo  = fbo_;
  auto const write_fbo = 0u;
  glBlitNamedFramebuffer(
    read_fbo, 
    write_fbo, 
    0, 0, width_, height_,  //  Source rectangle. 
    _x, _y, _w, _h,         //  Destination rectangle.
    _mask, 
    _filter
  );
}

void Fbo::draw(float _x, float _y) const noexcept {
  draw(
    _x, _y, width_, height_, 
    GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, 
    GL_NEAREST
  );
}

void Fbo::debugDraw(std::string_view const& _label) const noexcept {
  std::string const window_id{ 
    _label.empty() ? std::string("FBO textures ") : _label
  };

  ImGui::Begin( window_id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    for (auto &tex : textures_) {
      float const width  = 320.0f; 
      float const height = width / tex->ratio();
      imgui_utils::display_texture(tex->id, width, height);
    }
  }
  ImGui::End();
}

/* -------------------------------------------------------------------------- */
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
    glNamedFramebufferTexture(fbo_, _attachment, tex->id, 0 /*level*/);
    CHECK_GX_ERROR();
  }

  return tex;
}

/* -------------------------------------------------------------------------- */
