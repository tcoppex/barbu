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

  void setTexture(ResourceId resource_id); //

  void render(Camera const& camera);

  inline TextureHandle textureDiffuse() { return sky_map_; }
  inline TextureHandle textureIrradiance() { return irradiance_map_; }
  inline TextureHandle texturePrefilter() { return prefilter_map_; }
  inline TextureHandle textureBRDFLookup() { return brdf_lut_map_; }

  /* Spherical harmonics irradiances matrices. */
  inline glm::mat4 const* irradianceMatrices() const { return sh_matrices_.data(); }
  inline bool hasIrradianceMatrices() const noexcept { return has_sh_matrices_; }

 private:
  enum class RenderMode {
    Sky,
    Convolution,
    Prefilter, //
    kCount
  };

  void computeIntegratedBRDF();
  void computeConvolutionMaps(std::string const& basename);
  
  void render(RenderMode mode, Camera const& camera);

  struct {
    ProgramHandle cs_transform;             //< transform spherical map to cube map.
    ProgramHandle render;                   //< render skybox.
    ProgramHandle convolution;              //< convolute irradiance map.
    ProgramHandle prefilter;  //            //< prefilter specular map.
  } pgm_;

  MeshHandle cube_mesh_{nullptr}; //

  TextureHandle sky_map_;                   //< sky diffuse cubmap.
  TextureHandle irradiance_map_;            //< diffuse irradiance envmap.

  TextureHandle prefilter_map_;             //< prefiltered specular envmap.
  TextureHandle brdf_lut_map_;              //< BRDF LUT.

  Irradiance::SHMatrices_t sh_matrices_;    //< optional alternative for precomputed irradiance.
  bool has_sh_matrices_ = false;
};

// ----------------------------------------------------------------------------

#endif // BARBU_FX_SKYBOX_H_