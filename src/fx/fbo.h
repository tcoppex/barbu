#ifndef BARBU_FX_FBO_H_
#define BARBU_FX_FBO_H_

#include <vector>

#include "glm/vec4.hpp"

#include "core/graphics.h"
#include "memory/assets/texture.h"

// ----------------------------------------------------------------------------

//
// [ Work In Progress ] 
// Wrapper around a [OpenGL] Framebuffer object.
//
class Fbo {
 public:
  static constexpr char const* kDefaultFboTextureName{ "FBO::Texture" };
  
  static constexpr int32_t kDefaultInternalFormat = GL_RGBA8;
  static constexpr int32_t kDefaultMSAANumSamples = 0;
  static constexpr int32_t kDefaultTextureLevels  = 1;

  /* When set to true, will create a depth renderbuffer by default. */
  static constexpr bool kUseDepthBuffer = false;
  static constexpr int32_t kDefaultDepthFormat = GL_DEPTH_COMPONENT24;
 
 public:
  Fbo()
    : fbo_(0u)
    , renderbuffer_(0u)
    , width_(0)
    , height_(0)
  {}

  ~Fbo() {
    release();
  }

  /* Create the framebuffer with a default texture target attached to it, of type _internalFormat. 
   * When kUseDepthBuffer is true, will also create a default depth render buffer. */
  bool setup(int32_t _width, int32_t _height, int32_t _internalFormat = kDefaultInternalFormat);
  void release();

  /* Return true when the framebuffer is complete, false otherwise. */
  bool checkStatus() const noexcept;

  /* Bind the framebuffer for read / write. */ 
  void begin() const noexcept;
  void end() const noexcept;

  /* Create internal color texture attachments. */
  TextureHandle addColorAttachment(int32_t _internalFormat, int32_t _attachment_index) noexcept;
  
  /* Create internal color texture attachments incrementally. */
  TextureHandle addColorAttachment(int32_t _internalFormat) noexcept {
    return addColorAttachment(_internalFormat, textures_.size());  
  }

  /* Create internal special texture attachments. */
  TextureHandle addDepthAttachment(int32_t _internalFormat) noexcept;
  TextureHandle addStencilAttachment(int32_t _internalFormat) noexcept;
  TextureHandle addDepthStencilAttachment(int32_t _internalFormat) noexcept;

  /* Create a Renderbuffer attachment.
   * _attachment must be one of GL_DEPTH, GL_STENCIL, or GL_DEPTH_STENCIL.
   * _internalFormat must match _attachment type. */
  void addRenderbufferAttachment(int32_t _internalFormat, GLenum _attachment) noexcept;

  /* Clear internal buffers. */
  void clearColorBuffer(glm::vec4 const& _color, int32_t _attachment_index = 0) const noexcept;
  void clearDepthBuffer(float _depth) const noexcept;
  void clearStencilBuffer(int32_t _stencil) const noexcept;
  void clearDepthStencilBuffer(float _depth, int32_t _stencil) const noexcept;

  /* Return the texture for the given attachment, if any. */
  TextureHandle texture(GLenum _attachment) const noexcept;

  /* Return the color texture of the given attachment id, if any. */
  TextureHandle colorTexture(int32_t _attachment_index = 0) const noexcept;

  /* Return the depth or depthStencil texture, if any. */
  TextureHandle depthTexture() const noexcept;

  /* Return true when the underlying framebuffer object is initialized. */
  inline bool isInitialized() const noexcept {
    return fbo_ > 0u;
  }

  /* Return true when a renderbuffer is allocated. */
  inline bool hasRenderbuffer() const noexcept {
    return renderbuffer_ > 0u;
  }

  /* Draw the content of the framebuffer to the screen at the given coordinates. */
  void draw(float _x, float _y, float _w, float _h, GLbitfield _mask, GLenum _filter) const noexcept;
  void draw(float _x, float _y) const noexcept;

  /* Draw internal textures in a UI window for debugging. */
  void debugDraw(std::string_view const& _label = "") const noexcept;

  /* [tmp] return the GL identifier. */
  inline GLuint id() const noexcept {
    return fbo_;
  }

 private:
  /* Generic internal texture attachments. */
  TextureHandle addAttachment(int32_t _internalFormat, GLenum _attachment) noexcept;

 private:
  GLuint fbo_;                                        //< Framebuffer object.
  GLuint renderbuffer_;                               //< Renderbuffer (for depth / stencil).
  
  int32_t width_;                                     //< Framebuffer's width.
  int32_t height_;                                    //< Framebuffer's height.

  std::vector<TextureHandle> textures_;               //< Internal buffer textures.
  std::vector<GLenum> attachments_;                   //< LUTs for textures attachments.
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_FBO_H_