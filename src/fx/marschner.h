#ifndef BARBU_FX_MARSCHNER_H_
#define BARBU_FX_MARSCHNER_H_

#include <array>
#include "core/graphics.h"
class UIView;

// ----------------------------------------------------------------------------
//
// Interface to preprocess Marshner reflectance model lookup tables on the GPU.
//  
//    "Light Scattering from Human Hair Fibers", from Marschner et al. [2003]
//
class Marschner {
 public:
  constexpr static int kNumLUTs = 2;
  constexpr static int kTextureResolution = 256;
  constexpr static int kComputeBlockSize  = 16; // 
  constexpr static GLenum kTextureFormat  = GL_RGBA16F;

  struct ShadingParameters_t {
    // Fiber Properties.
    float eta             = 1.55f;
    float absorption      = 0.20f;   
    float eccentricity    = 0.85f;

    // Surface Properties.
    float ar              = -10.0f;
    float br              = 5.0f;

    // Glints.
    float glintScale      = 0.5f;
    float azimuthalWidth  = 10.0f;
    float deltaCaustic    = 0.2f;
    float deltaHm         = 0.5f;

    bool operator==(ShadingParameters_t const& o) {
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
  };

  struct Parameters_t {
    ShadingParameters_t shading;
    GLuint *tex_ptr[kNumLUTs]{ nullptr, nullptr };
  };

 public:
  Marschner() = default;
  ~Marschner();

  void init();
  void update(bool bForceUpdate = false);
  void generate();

  UIView* view() const {
    return ui_view_;
  }

  void bind_lookups(int baseUnit = 0) {
    for (int i=0; i<kNumLUTs; ++i) {
      glBindTextureUnit(baseUnit + i, tex_[i]);
    }
  }

  void unbind_lookups(int baseUnit = 0) {
    for (int i=0; i<kNumLUTs; ++i) {
      glBindTextureUnit(baseUnit + i, 0);
    }
  }

 private:
  UIView *ui_view_ = nullptr;
  Parameters_t params_;
  ShadingParameters_t lastShadingParams_;

  std::array<GLuint, kNumLUTs> pgm_;
  std::array<GLuint, kNumLUTs> tex_;

 private:
  Marschner(Marschner const&) = delete;
  Marschner(Marschner&&) = delete;
};

// ----------------------------------------------------------------------------


#endif // BARBU_FX_MARSCHNER_H_