#ifndef BARBU_FX_PROBE_H_
#define BARBU_FX_PROBE_H_

#include <array>
#include "glm/glm.hpp"

#include "memory/assets/texture.h"
#include "memory/assets/program.h"

// ----------------------------------------------------------------------------

//
// Capture HDR cubemap. [Work in Progress]
//
class Probe {
 public:
  // View matrices for each faces of an axis aligned cube.
  static const std::array<glm::mat4, 6> kViewMatrices;

 public:
  Probe() = default;

 private:
  ProgramHandle program_;
  TextureHandle cubemap_;
  int32_t resolution_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_PROBE_H_