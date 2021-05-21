#ifndef BARBU_FX_PROBE_H_
#define BARBU_FX_PROBE_H_

#include <functional>
#include "glm/glm.hpp"

#include "core/camera.h"
#include "memory/enum_array.h"
#include "memory/assets/texture.h"

// ----------------------------------------------------------------------------

// Enum to face indices of a cube map.
enum class CubeFace {
  PosX = 0,
  NegX,
  PosY,
  NegY,
  PosZ,
  NegZ,

  kCount
};

// ----------------------------------------------------------------------------

//
// Capture HDR cubemap. [Work in Progress]
//
class Probe {
 public:
  static constexpr int32_t kDefaultCubemapResolution = 512;

  // Array to iter over cubeface enums.
  static EnumArray<CubeFace, CubeFace> const kIterFaces;

 public:
  using DrawCallback_t = std::function<void(Camera const&)>;

  Probe()
    : resolution_(0u)
    , fbo_(0u)
    , renderbuffer_(0u)
    , texture_(nullptr)
  {}

  ~Probe() {
    release();
  }

  void init(int32_t const resolution = kDefaultCubemapResolution);
  void release();

  void begin();
  void end();

  // Prepare the face to render and return its view matrices.
  Camera const& setup_face(CubeFace face);

  // Update all 6 faces of the cubemap by providing a draw callback expecting 
  // a view projection matrix. 
  void capture(DrawCallback_t draw_cb);

  inline int32_t resolution() const { return resolution_; }
  inline TextureHandle texture() const { return texture_; }

 private:
  // Probe camera view controller.
  class ViewController final : public Camera::ViewController {
   public:
    // View matrices for each faces of an axis aligned cube.
    static EnumArray<glm::mat4, CubeFace> const kViewMatrices;

    virtual ~ViewController() {}

    inline void set_face(CubeFace face) { face_ = face; }
    inline void get_view_matrix(float *m) final { memcpy(m, glm::value_ptr(kViewMatrices[face_]), 16 * sizeof(float)); } 
    inline glm::vec3 target() const final { return glm::vec3(0.0f); }

   private:
    CubeFace face_ = CubeFace::PosX;
  } view_controller_;
  
  // Shared probe camera. 
  static Camera sCamera;

  int32_t resolution_;

  // [todo : use generic objects]
  uint32_t fbo_;
  uint32_t renderbuffer_;
  TextureHandle texture_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_PROBE_H_