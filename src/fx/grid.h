#ifndef BARBU_FX_GRID_H_
#define BARBU_FX_GRID_H_

#include "glm/mat4x4.hpp"

#include "memory/assets/mesh.h"
#include "memory/assets/program.h"
class Camera;

// ----------------------------------------------------------------------------

// The grid is a special / debug rendering object to visualize our location
// on the 3d space.
// 
// The update method is used to allow the grid to face the camera on special
// view, as well as to blend between views. 
class Grid {
 public:
  static constexpr int kMainGridHalfRes = 5;
  static constexpr int kSubGridStep     = 4;
  static constexpr int kGridNumCell     = 2 * kMainGridHalfRes * kSubGridStep;
  
  static constexpr float kGridScale     = 1.0f;
  static constexpr float kGridValue     = 0.60f;
  static constexpr float kGridAlpha     = 0.95f;
  static constexpr bool kEnableSideGrid = true;
  
 public:
  Grid() = default;

  void init();
  void deinit();

  // Update transform and alpha based on camera settings.
  void update(float const dt, Camera const& camera);

  // Render the grid with antialiased lines.
  void render(Camera const& camera);

 private:
  MeshHandle mesh_;
  ProgramHandle pgm_;

  glm::mat4 matrix_;
  float alpha_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_GRID_H_