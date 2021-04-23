#ifndef SHADERS_SHARED_LIGHTING_INC_CLASSICAL_GLSL_
#define SHADERS_SHARED_LIGHTING_INC_CLASSICAL_GLSL_

#include "shared/lighting/interop.h"

// ----------------------------------------------------------------------------

// Non PBR classical lighting model (Lambert diffuse + Phong specular).

// ----------------------------------------------------------------------------

// Diffuse and specular model.
vec3 apply_complete_light(in vec3 N, in vec3 L, in vec3 R, in vec3 V, in vec3 color, float exponent) {
  const float NdotL = dot(N, L);
  const float RdotV = dot(R, V); // blinn = N * H

  const float Kd = max(0.0, NdotL);
  const float Ks = pow( max(RdotV, 0), exponent) * (1.0 - step(NdotL, 0.0));
  
  const vec3 lighting = color * (Kd + Ks);

  return clamp(lighting, vec3(0), vec3(1));
}

vec3 apply_light(in vec3 N, in vec3 L, in vec3 V, in vec3 color) {
  const float kSpecularExponent = 0.95f; //

#if 1
  // PHONG
  const vec3 R = reflect(N, L);
  return apply_complete_light( N, L, R, V, color, kSpecularExponent);
#else
  // BLINN [fixme]
  const vec3 H = normalize(V + L);
  return apply_complete_light( N, L, N, H, color, kSpecularExponent);
#endif
}

// Simple Lambert diffuse model.
vec3 apply_light(in vec3 N, in vec3 L, in vec3 color) {
  return color * max(0.0, dot(N, L));
}

// ----------------------------------------------------------------------------

// Return a vec4 with :
//  * XYZ as normalized direction to light
//  * W as attenuation factor 
vec4 get_pointlight_params(in vec3 frag_to_light, in float radius_squared) {
  const float lds = dot(frag_to_light, frag_to_light);
  const float atten = 1.0 - clamp( lds / radius_squared, 0., 1. );
  return vec4( frag_to_light * inversesqrt(lds), atten);
}

// ----------------------------------------------------------------------------

// DIFFUSE & SPECULAR.

vec3 apply_directional_light(in LightInfo_t light, in vec3 N, in vec3 V) {
  const vec3 L = - normalize(light.direction.xyz);
  return apply_light(N, L, V, light.color.rgb) 
       * light.color.a
       ;
}

vec3 apply_point_light(in LightInfo_t light, in vec3 pos, in vec3 N, in vec3 V, float radius) {
  const vec4 L = get_pointlight_params( light.position.xyz - pos, radius * radius);
  return apply_light(N, L.xyz, V, light.color.rgb)
       * light.color.w
       * L.w
       ;
}

// ----------------------------------------------------------------------------

// DIFFUSE ONLY.

vec3 apply_directional_light(in LightInfo_t light, in vec3 N) {
  const vec3 L = - normalize(light.direction.xyz);
  return apply_light( N, L, light.color.rgb)
       * light.color.a
       ;
}

vec3 apply_point_light(in LightInfo_t light, in vec3 pos, in vec3 N, float radius) {
  const vec4 L = get_pointlight_params( light.position.xyz - pos, radius * radius);  
  return apply_light( N, L.xyz, light.color.rgb)
       * light.color.w
       * L.w 
       ;
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_LIGHTING_INC_CLASSICAL_GLSL_
