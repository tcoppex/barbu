#ifndef BARBU_FX_SKYBOX_H_
#define BARBU_FX_SKYBOX_H_

#include "glm/mat4x4.hpp"

#include "memory/assets/mesh.h"
#include "memory/assets/texture.h"
class Camera;

// (already defined in irradiance_env_map.h)
using IrradianceMatrices_t = std::array<glm::mat4, 3>; //

// ----------------------------------------------------------------------------

class Skybox {
 public:
  Skybox() = default;

  void init();
  
  void deinit();

  void render(Camera const& camera);

  inline glm::mat4 const* irradiance_matrices() const { return shMatrices_.data(); }

 private:
  MeshHandle cubemesh_;
  
  uint32_t pgm_;
  TextureHandle skytex_;

  // Skybox irradiance spherical harmonics matrices.
  IrradianceMatrices_t shMatrices_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_SKYBOX_H_