#ifndef SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_
#define SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_

struct Material_t {
  vec4 color;
  float metallic;
  float roughness;
  vec3 irradiance;  
};

#endif // SHADERS_SHARED_STRUCTS_INC_MATERIAL_GLSL_