#ifndef BARBU_FX_POSTPROCESS_POSTPROCESS_H_
#define BARBU_FX_POSTPROCESS_POSTPROCESS_H_

#include <array>

#include "glm/vec2.hpp"
#include "core/graphics.h"
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
    GL_RGBA8, //
    GL_DEPTH_COMPONENT24,
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
    , vao_(0u)
    , w_(0)
    , h_(0)
  {}

  void init();
  void deinit();

  /* To call when the camera / framebuffer has been resized. */
  void setupTextures(Camera const& camera);

  void begin();
  void end(Camera const& camera); //

  inline GLuint bufferTextureID(BufferTextureName_t const name) const noexcept { 
    return buffers_[current_buffer_][name];
  }

  inline bool enabled() const noexcept { return bEnable_; }
  inline void toggle(bool status) noexcept { bEnable_ = status; }

 private:
  void createTextures();
  void releaseTextures();

  /* Post capture processing. */
  void applyEffects(Camera const& camera);

  /* Render screen quad for the final composition. */
  void renderScreen();

 private:
  using InternalBuffers_t = std::array<GLuint, kNumBufferTextureName>;

  bool bEnable_;
  bool bTextureInit_;

  // Input buffers.
  std::array<GLuint, kNumBuffers> fbos_{0};  //
  std::array<InternalBuffers_t, kNumBuffers> buffers_{};
  int current_buffer_;

  // Mapscreen VAO.
  GLuint vao_;
  
  // Composition programs.
  struct {
    ProgramHandle mapscreen;
    ProgramHandle lindepth;
  } pgm_;

  // Output textures.
  struct {
    GLuint lindepth_r32f  = 0u; //
    GLuint ext_ao_r32f    = 0u; //
  } out_;

  // Buffer resolution.
  int32_t w_;
  int32_t h_;

  // Linearized depth resolution.
  glm::vec2 lindepth_res_; //

  // Sub-effects.
  HBAO ssao_;

 private:
  Postprocess(Postprocess const&) = delete;
  Postprocess(Postprocess&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_POSTPROCESS_POSTPROCESS_H_
