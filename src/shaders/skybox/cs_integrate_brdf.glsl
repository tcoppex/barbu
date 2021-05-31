#version 430 core
precision highp float;

#include "shared/inc_maths.glsl" // for importance_sample_GGX
#include "shared/lighting/inc_pbr.glsl" // for gf_SmithGGX

// ----------------------------------------------------------------------------
//
// Calculate the 2D look-up table for the environment BRDF.
//  X is the cosine of the view angle, ie. N dot V.
//  Y is the roughness of the material.
//
// This could be further improved by dividing samples by threads (in grid Z).
//
// Reference :
//  * "Real Shading in Unreal Engine 4" - Brian Karis 
//
// ----------------------------------------------------------------------------

uniform int uResolution;
uniform int uNumSamples = 1024;

writeonly 
uniform layout(rgba16f) image2D uDstImg;

// ----------------------------------------------------------------------------

vec2 integrate_brdf(float n_dot_v, float roughness) {
  const float inv_samples   = 1.0f / float(uNumSamples);
  const mat3 basis_ws       = basis_from_view(vec3(0.0, 0.0, 1.0));
  const float roughness_sqr = pow( roughness, 2.0);

  const vec3 V = vec3(sqrt(1.0 - pow(n_dot_v, 2.0)), 0.0, n_dot_v);

  vec2 brdf = vec2(0.0);

  for (int i = 0; i < uNumSamples; ++i) {
    const vec2 pt = hammersley2d( i, inv_samples);
    const vec3 H  = importance_sample_GGX( basis_ws, pt, roughness_sqr);
    const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

    const float n_dot_l = max( L.z, 0.0);

    if (n_dot_l > 0.0) {
      const float n_dot_h = max( H.z, 0.0);
      const float v_dot_h = max(dot(V, H), 0.0);

      const float G     = gf_SmithGGX( n_dot_v, n_dot_l, roughness_sqr);
      const float G_Vis = G * v_dot_h / (n_dot_h * n_dot_v);
      const float Fc    = pow( 1.0 - v_dot_h, 5.0);

      brdf += vec2(1.0 - Fc, Fc) * G_Vis;
    }
  }
  brdf *= inv_samples;

  return brdf;
}

// ----------------------------------------------------------------------------

#ifndef BLOCK_DIM
  #define BLOCK_DIM      16
#endif

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM) in;
void main()
{
  const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

  // Check boundaries.
  if (!all(lessThan(coords, uvec2(uResolution)))) {
    return;
  }

  const vec2 uv = vec2(coords.xy) / float(uResolution);
  vec2 data = integrate_brdf(uv.x, uv.y);

  imageStore( uDstImg, coords, vec4( data, 0.0, 1.0));
}

// ----------------------------------------------------------------------------