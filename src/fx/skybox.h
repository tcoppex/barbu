#ifndef BARBU_FX_SKYBOX_H_
#define BARBU_FX_SKYBOX_H_

#include "glm/mat4x4.hpp"

#include "memory/assets/mesh.h"
#include "memory/assets/texture.h"
#include "memory/assets/program.h"
class Camera;

// for IraddianceMatrices
#include "fx/irradiance_env_map.h"

// ----------------------------------------------------------------------------

class Skybox {
 public:
  Skybox() = default;

  void init();
  
  void deinit();

  void render(Camera const& camera);

  uint32_t get_texture_id() const { return skytex_ ? skytex_->id : 0u; } //

  inline glm::mat4 const* irradiance_matrices() const { return shMatrices_.data(); }

 private:
  MeshHandle cubemesh_;
  ProgramHandle pgm_;
  TextureHandle skytex_;

  ProgramHandle pgm_transform_;

  // Skybox irradiance spherical harmonics matrices.
  IrradianceMatrices_t shMatrices_;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_SKYBOX_H_