#ifndef BARBU_FX_SKYBOX_H_
#define BARBU_FX_SKYBOX_H_

#include "glm/mat4x4.hpp"

#include "memory/assets/mesh.h"
#include "memory/assets/texture.h"
#include "memory/assets/program.h"
class Camera;

#include "fx/irradiance.h"

// ----------------------------------------------------------------------------

//
// Manages HDR environment maps and their irradiance contribution.
//
// Irradiance contributions are stored in both spherical harmonics
// matrices and as cubemap. 
//
class Skybox {
 public:
  Skybox() = default;

  void init();
  void deinit();

  void render(Camera const& camera);

  TextureHandle texture() { return sky_map_; }
  TextureHandle irradiance_map() { return irradiance_map_; }

  inline glm::mat4 const* irradiance_matrices() const { return sh_matrices_.data(); }

 private:
  void setup_texture(/*ResourceInfo info*/);

  enum class RenderMode {
    Sky,
    Convolution,
  };
  void render(RenderMode mode, Camera const& camera);

  struct {
    ProgramHandle cs_transform;             //< transform spherical map to cube map.  
    ProgramHandle render;                   //< render skybox.
    ProgramHandle convolution;              //< convolute irradiance map.  
  } pgm_;

  MeshHandle cube_mesh_; //

  TextureHandle sky_map_;                   //< sky cubemap.
  TextureHandle irradiance_map_;            //< irradiance envmap.

  Irradiance::SHMatrices_t sh_matrices_;    //< irradiances spherical harmonics matrices.
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_SKYBOX_H_