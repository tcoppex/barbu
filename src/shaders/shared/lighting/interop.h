#ifndef SHADERS_SHARED_LIGHTING_INTEROP_H_
#define SHADERS_SHARED_LIGHTING_INTEROP_H_

#ifdef __cplusplus
#include "glm/glm.hpp"
using namespace glm;
#endif

// ----------------------------------------------------------------------------

#define LIGHT_TYPE_DIRECTIONAL      0
#define LIGHT_TYPE_POINT            1
#define LIGHT_TYPE_SPOT             2

// Data in a ShaderStorage buffer must be layed out using atomic type,
// ie. float[3] instead of vec3, to avoid unwanted padding, otherwise use vec4.

/* User defined light parameters. */
struct LightInfo_t {
  vec4 position;        //< XYZ Position + W Type
  vec4 color;           //< XYZ RGB Color + W intensity
  vec4 direction; //
  vec4 params;
};

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_LIGHTING_INTEROP_H_
