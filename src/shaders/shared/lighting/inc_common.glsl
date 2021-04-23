#ifndef SHADERS_SHARED_LIGHTING_INC_COMMON_GLSL_
#define SHADERS_SHARED_LIGHTING_INC_COMMON_GLSL_

#include "shared/lighting/interop.h"          // LightInfo_t
#include "shared/structs/inc_fraginfo.glsl"   // FragInfo_t
#include "shared/inc_constants.glsl"          // Epsilon()

// ----------------------------------------------------------------------------

const int kMaxNumLights = 4;

uniform LightInfo_t uLightInfos[kMaxNumLights];
uniform int uNumLights = 0;

// ----------------------------------------------------------------------------

/* Fragment specific light parameters. */
struct FragLight_t {
  vec3 L;               //< Fragment to light vector (normalized).
  vec3 H;               //< Half vector (normalized).
  float n_dot_l;
  float n_dot_h;
  vec3 radiance;        //< light color after attenuation.
};

/* Combine LightInfo & FragmentInfo to determine Fragment's specific Light parameters. */
FragLight_t get_fraglight_params(in LightInfo_t light_info, in FragInfo_t frag_info) {
  FragLight_t light;

  const int light_type = int(light_info.position.w);

  if (light_type == LIGHT_TYPE_POINT) 
  {
    const vec3 to_light = light_info.position.xyz - frag_info.P; 
    const float d_sqr   = max(dot( to_light, to_light), Epsilon());
    light.L             = to_light * inversesqrt(d_sqr);
    light.radiance      = light_info.color.rgb / d_sqr;
  } 
  else if (light_type == LIGHT_TYPE_DIRECTIONAL) 
  {
    light.L             = - light_info.direction.xyz; // (assumed normalized)
    light.radiance      = light_info.color.rgb;
  }
  
  // Apply intensity to radiance.
  light.radiance *= light_info.color.w;

  // Half vector.
  light.H = normalize( light.L + frag_info.V );
  
  // Saturated dot products.
  light.n_dot_l = clamp(dot(frag_info.N, light.L), 0, 1);
  light.n_dot_h = clamp(dot(frag_info.N, light.H), 0, 1);

  return light;
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_LIGHTING_INC_COMMON_GLSL_
