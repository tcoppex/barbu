#ifndef BARBU_FX_GRID_H_
#define BARBU_FX_GRID_H_

#include "glm/mat4x4.hpp"

#include "memory/assets/mesh.h"
class Camera;

// ----------------------------------------------------------------------------

// The grid is a special / debug rendering object to visualize our location
// on the 3d space.
// 
// The update method is used to allow the grid to face the camera on special
// view, as well as to blend between views. 
class Grid {
 public:
  static constexpr int kGridNumCell     = 32;
  static constexpr float kGridScale     = 1.0f;
  static constexpr float kGridAlpha     = 0.92f;
  static constexpr bool kEnableSideGrid = true;

 public:
  Grid() = default;

  void init();
  
  void deinit();

  // Update transform and alpha based on camera settings.
  void update(float const dt, Camera const& camera);

  void render(Camera const& camera);

 private:
  MeshHandle mesh_;

  uint32_t pgm_;

  glm::mat4 matrix_;
  float alpha_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_GRID_H_