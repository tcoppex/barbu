#ifndef BARBU_UTILS_GIZMO_H_
#define BARBU_UTILS_GIZMO_H_

#include "glm/glm.hpp"
#include "im3d/im3d.h"

#include "core/graphics.h"
#include "memory/assets/program.h"
class Camera;

// ----------------------------------------------------------------------------

//
// Wrapper to handle im3d gizmos.
//
class Gizmo {
 public:
  static constexpr bool kbFlipGizmoWhenBehind = false;
  static constexpr bool kDefaultGlobal        = false;
  static constexpr float kGizmoScaling        = 0.95f;
  static constexpr float kTranslationSnapUnit = 0.5f;
  static constexpr float kRotationSnapUnit    = glm::radians(30.0f);
  static constexpr float kScalingSnapUnit     = 0.5f;

 public:
  Gizmo()
    : vao_(0u)
    , vbo_(0u)
  {}

  void init();
  void deinit();

  void begin_frame(float dt, Camera const& camera);
  void end_frame(Camera const& camera);

 private:
  GLuint vao_;
  GLuint vbo_;

  struct {
    ProgramHandle points;
    ProgramHandle lines;
    ProgramHandle triangles;
  } pgm_;

  // States of transforms.
  bool bLastTranslation_ = true;
  bool bLastRotation_    = false;

 private:
  Gizmo(Gizmo const&) = delete;
  Gizmo(Gizmo&&) = delete;
};

// ----------------------------------------------------------------------------

#endif // BARBU_UTILS_GIZMO_H_