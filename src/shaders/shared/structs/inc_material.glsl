#ifndef SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_
#define SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_

struct Material_t {
  vec4 color;
  float metallic;
  float roughness;
  float ao;
  vec3 emissive;

  vec3 irradiance;
  vec3 prefiltered;
  vec2 BRDF;
};

#endif // SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_