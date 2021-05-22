#ifndef BARBU_FX_PROBE_H_
#define BARBU_FX_PROBE_H_

#include <functional>
#include "glm/glm.hpp"

#include "core/camera.h"
#include "memory/enum_array.h"
#include "memory/assets/texture.h"

// ----------------------------------------------------------------------------

// Enum to face indices of a cubemap.
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
// Capture HDR environment cubemap.
//
class Probe {
 public:
  static constexpr int32_t kDefaultCubemapResolution = 512;

  // Array to iter over cubeface enums.
  static EnumArray<CubeFace, CubeFace> const kIterFaces;

  // View matrices for each faces of an axis aligned cube.
  static EnumArray<glm::mat4, CubeFace> const kViewMatrices;

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

  void init(int32_t const resolution = kDefaultCubemapResolution, bool bUseDepth = true);
  void release();

  // Update all 6 faces of the cubemap by providing a draw callback expecting 
  // a view projection matrix.
  void capture(DrawCallback_t draw_cb);

  inline int32_t resolution() const { return resolution_; }
  inline TextureHandle texture() const { return texture_; }

 private:
  // Setup the internal framebuffer for capture.
  void begin();
  void end();

  // Prepare the face to render, shall be called between begin() / end().
  void setup_face(CubeFace face);

  // Probe camera view controller.
  class ViewController final : public Camera::ViewController {
   public:
    virtual ~ViewController() {}

    inline void set_face(CubeFace face) { face_ = face; }
    inline void get_view_matrix(float *m) final { memcpy(m, glm::value_ptr(Probe::kViewMatrices[face_]), 16 * sizeof(float)); } 
    inline glm::vec3 target() const final { return glm::vec3(0.0f); } //

   private:
    CubeFace face_ = CubeFace::PosX;
  } view_controller_;
   
  static Camera sCamera;                                //< Shared probe camera.

  int32_t resolution_;                                  //< Framebuffer resolution.
  uint32_t fbo_; //                                     //< Framebuffer object.
  uint32_t renderbuffer_; //                            //< Renderbuffer (for depth).
  TextureHandle texture_;                               //< Environment map.
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_PROBE_H_