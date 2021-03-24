#ifndef BARBU_FX_POSTPROCESS_HBAO_H_
#define BARBU_FX_POSTPROCESS_HBAO_H_

#include <array>
#include "glm/vec4.hpp"
#include "core/graphics.h"
class Camera;

// ----------------------------------------------------------------------------

//
// Horizontal Based Ambient Occlusion (HBAO) post-process pass.
// (original algorithm from Louis Bavoil, Nvidia)
//
class HBAO {
 public:
  // Base and ui controllable params.
  struct UIParameters_t {
    float radius         = 0.05f;
    float blur_radius    = 5.8f;
    float blur_sharpness = 10.0f;
    float angle_bias     = 0.240f;
    //float ao_res_scale   = 1.0f; 
  };

 private:
  static constexpr float kDefaultScaling{ 0.5f };

 public:
  HBAO() = default;

  void init(Camera const& camera, float const scaling = kDefaultScaling);
  void deinit();

  void process(Camera const& camera, GLuint const tex_linear_depth, GLuint &tex_ao_out);

 private:
  enum TextureName_t {
    AO_X,
    AO_XY,
    BLUR_AO_X,
    BLUR_AO_XY,
    kNumTextureName
  };

  static constexpr std::array<GLenum, kNumTextureName> kTextureFormats{
    GL_R32F, 
    GL_RG16F, 
    GL_RG16F, 
    GL_R32F,
  };

  static const int kHBAOTileWidth;
  static const int kBlurRadius;

  void init_textures();
  void init_shaders();

  void update_parameters(Camera const& camera);

  void run_kernel_hbao(GLuint const tex_linear_depth);
  void run_kernel_blur_ao();

  std::array<GLuint, kNumTextureName> textures_;

  struct {
    GLuint ssao = 0;
    GLuint blur_x;
    GLuint blur_y;
  } pgm_;

  UIParameters_t ui_params_;

  struct FinalParameters_t {
    glm::vec4 full_resolution;  // XY res, ZW inv res.
    glm::vec4 ao_resolution;    // XY res, ZW inv res.
    glm::vec4 focal_length;     // XY fl, ZW inv fl
    glm::vec4 uv_to_view;       // XY A, ZW B
    
    float radius_squared;
    
    float tan_angle_bias;
    float pow_exponent;
    float strength;

    float blur_depth_threshold;
    float blur_falloff;
  } params_;

 private:
  HBAO(HBAO const&) = delete;
  HBAO(HBAO&&) = delete;

 public:
  friend class Postprocess; // [for debug display only]
};

// ----------------------------------------------------------------------------

#endif  // BARBU_FX_POSTPROCESS_HBAO_H_
