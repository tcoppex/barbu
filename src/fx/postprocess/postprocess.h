#ifndef BARBU_FX_POSTPROCESS_POSTPROCESS_H_
#define BARBU_FX_POSTPROCESS_POSTPROCESS_H_

#include <array>

#include "glm/vec2.hpp"

#include "core/graphics.h"
#include "fx/fbo.h"
#include "fx/postprocess/hbao.h"

class Camera;

// ----------------------------------------------------------------------------

// [ should be rewrote entirely ]
//
//  Postprocess class capture a framebuffer and apply post effect to it.
//  Finally rendering it as a screen-aligned quad.
//  It is double buffered to use its current buffers on the next frame.
//
//  Right now it automatically linearized depth afterwards.
//
//  In a future version it will probably be internal to a camera / projector,
//  with post effects but no mandatory screen rendering but with linked composition
//  passes.
//
//  TODO : the final composition should be put after all passes, not only the post process
//          on solid objects.
//
class Postprocess {
 public:
  enum BufferTextureName_t {
    COLOR_RGBA8,
    DEPTH,

    kNumBufferTextureName
  };

  // Internal format of buffer textures.
  static constexpr std::array<GLenum, kNumBufferTextureName> kBufferTextureFormats{
    GL_RGBA8, //                     // GL_COLOR_ATTACHMENT0
    GL_DEPTH_COMPONENT24,            // GL_DEPTH_ATTACHMENT
  };

  // Number of copy for N-buffering.
  static constexpr int kNumBuffers{ 2 };

  // Linear depth internal format.
  static constexpr GLenum kLinearDepthFormat{ GL_R32F };

 public:
  Postprocess()
    : bEnable_(true)
    , bTextureInit_(false)
    , current_buffer_(0)
    , width_(0)
    , height_(0)
  {}

  void init();
  void deinit();

  /* To call when the camera / framebuffer has been resized. */
  void setupTextures(Camera const& camera);

  /**/
  void begin();
  void end(Camera const& camera); //

  /**/
  GLuint bufferTextureID(BufferTextureName_t const name) const noexcept;

  /**/
  inline bool enabled() const noexcept { return bEnable_; }
  inline void toggle(bool status) noexcept { bEnable_ = status; }

  /* Render internal textures in UI for debugging. */
  void debugDraw(std::string_view const& _label = "") const noexcept;

 private:
  void createTextures();
  void releaseTextures();

  /* Post capture processing. */
  void applyEffects(Camera const& camera);

  /* Render screen quad for the final composition. */
  void renderScreen();

  /* Return the current framebuffer object. */
  inline Fbo const& currentFBO() const noexcept { 
    return FBOs_[current_buffer_];
  }

 private:
  using InternalBuffers_t = std::array<GLuint, kNumBufferTextureName>;

  bool bEnable_;
  bool bTextureInit_;

  // Input buffers.
  std::array<Fbo, kNumBuffers> FBOs_;
  int current_buffer_;
  int32_t width_;
  int32_t height_;

  // Screen mapping.
  struct {
    ProgramHandle pgm;
    GLuint vao = 0u;
  } mapscreen_;

  // Linearize depth.
  struct {
    ProgramHandle pgm;
    TextureHandle tex;
    glm::vec2 resolution;
  } lindepth_;

  // Sub-effects.
  HBAO ssao_;
  
  // Output textures.
  struct {
    GLuint ao = 0u;
  } output_texid_;

 private:
  Postprocess(Postprocess const&) = delete;
  Postprocess(Postprocess&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_POSTPROCESS_POSTPROCESS_H_
