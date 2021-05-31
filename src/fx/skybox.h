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
// * Irradiance contributions are stored either as spherical harmonics matrices 
//   or as cubemap. 
//
class Skybox {
 public:
  Skybox() = default;

  void init();
  void deinit();

  void setup_texture(ResourceId resource_id); //

  void render(Camera const& camera);

  TextureHandle texture() { return sky_map_; }

  TextureHandle irradiance_map() { return irradiance_map_; }
  
  TextureHandle prefilter_map() { return prefilter_map_; }
  TextureHandle brdf_lut_map() { return brdf_lut_map_; }

  inline glm::mat4 const* irradiance_matrices() const { return sh_matrices_.data(); }

  inline bool has_irradiance_matrice() const { return has_sh_matrices_; }

 private:
  enum class RenderMode {
    Sky,
    Convolution,
    Prefilter, //
  };

  void calculate_integrated_brdf();

  void calculate_convolution_envmaps(std::string const& basename);

  void render(RenderMode mode, Camera const& camera);

  struct {
    ProgramHandle cs_transform;             //< transform spherical map to cube map.
    ProgramHandle render;                   //< render skybox.
    ProgramHandle convolution;              //< convolute irradiance map.
    ProgramHandle prefilter;  //            //< prefilter specular map.
  } pgm_;

  MeshHandle cube_mesh_ = nullptr; //

  TextureHandle sky_map_;                   //< sky diffuse cubmap.
  TextureHandle irradiance_map_;            //< diffuse irradiance envmap.

  TextureHandle prefilter_map_;             //< prefiltered specular envmap.
  TextureHandle brdf_lut_map_;              //< BRDF LUT.

  Irradiance::SHMatrices_t sh_matrices_;    //< irradiances spherical harmonics matrices.
  bool has_sh_matrices_ = false;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_SKYBOX_H_