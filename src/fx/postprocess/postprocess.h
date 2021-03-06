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

  // To call when the camera / framebuffer has been resized.
  void setup_textures(Camera const& camera);

  void begin();
  void end(Camera const& camera); //

  GLuint buffer_texture(BufferTextureName_t const name) { 
    return buffers_[current_buffer_][name];
  }

  inline bool is_enable() const { return bEnable_; }
  inline void toggle(bool status) { bEnable_ = status; }

 private:
  using InternalBuffers_t = std::array<GLuint, kNumBufferTextureName>;

  void create_textures();
  void release_textures();

  // Post capture processing.
  void post_effects(Camera const& camera);

  // Render screen quad for final composition.
  void render_screen();


  bool bEnable_;
  bool bTextureInit_;

  // Input buffers.
  GLuint fbos_[kNumBuffers];  //
  InternalBuffers_t buffers_[kNumBuffers];
  int current_buffer_;

  // Mapscreen VAO.
  GLuint vao_; //
  
  // Composition programs.
  struct {
    ProgramHandle mapscreen;
    ProgramHandle lindepth;
  } pgm_;

  // Outputs textures.
  struct {
    GLuint lindepth_r32f = 0; //
    GLuint ext_ao_r32f = 0; //
  } out_;

  int32_t w_, h_;
  glm::vec2 lindepth_res_; //

  // Sub-effects.
  HBAO ssao_;

 private:
  Postprocess(Postprocess const&) = delete;
  Postprocess(Postprocess&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_POSTPROCESS_POSTPROCESS_H_
