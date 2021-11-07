#ifndef BARBU_FX_MARSCHNER_H_
#define BARBU_FX_MARSCHNER_H_

#include <array>
#include <memory>

#include "core/graphics.h"

#include "memory/assets/program.h" //
#include "memory/assets/texture.h" //
class UIView;

// ----------------------------------------------------------------------------
//
// Interface to preprocess Marshner reflectance model lookup tables on the GPU.
//  
//    "Light Scattering from Human Hair Fibers", from Marschner et al. [2003]
//
class Marschner {
 public:
  std::shared_ptr<UIView> ui_view = nullptr;

 public:
  constexpr static int kTextureResolution = 128;
  constexpr static int kComputeBlockSize  = 16; //
  constexpr static GLenum kTextureFormat  = GL_RGBA16F; //

  constexpr static int kNumLUTs = 2;
  constexpr static float kInvTextureResolution = 1.0f / static_cast<float>(kTextureResolution);

  struct ShadingParameters_t {
    // Fiber Properties.
    float eta             = 1.55f;
    float absorption      = 0.20f;   
    float eccentricity    = 0.85f;

    // Surface Properties.
    float ar              = -5.0f;
    float br              = +5.0f;

    // Glints.
    float glintScale      = 0.5f;
    float azimuthalWidth  = 10.0f;
    float deltaCaustic    = 0.2f;
    float deltaHm         = 0.5f;

    bool operator==(ShadingParameters_t const& o) const noexcept {
      return (eta == o.eta)
          && (absorption == o.absorption)
          && (eccentricity == o.eccentricity)
          && (ar == o.ar)
          && (br == o.br)
          && (glintScale == o.glintScale)
          && (azimuthalWidth == o.azimuthalWidth)
          && (deltaCaustic == o.deltaCaustic)
          && (deltaHm == o.deltaHm)
          ;
    }
    bool operator!=(ShadingParameters_t const& o) const noexcept {
      return !(*this == o);
    }
  };

  struct Parameters_t {
    ShadingParameters_t shading;
    GLuint const* tex_ptr[kNumLUTs]{ nullptr, nullptr };
  };

 public:
  Marschner() = default;
  ~Marschner();

  void init();
  void update(bool bForceUpdate = false);
  void generate();

  void bindLUTs(int baseUnit = 0);
  void unbindLUTs(int baseUnit = 0);

 private:
  Parameters_t params_;
  ShadingParameters_t previous_shading_params_;

  std::array<ProgramHandle, kNumLUTs> programs_;
  std::array<TextureHandle, kNumLUTs> textures_;

 private:
  Marschner(Marschner const&) = delete;
  Marschner(Marschner&&) = delete;
};

// ----------------------------------------------------------------------------


#endif // BARBU_FX_MARSCHNER_H_