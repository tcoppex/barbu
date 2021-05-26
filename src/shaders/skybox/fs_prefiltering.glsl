#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------
//
// Calculate the prefilter convolution of an environment map.
// Increasing level of the cubemap correspond to a higher rough reflection level. 
//
// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inView;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform samplerCube uCubemap;
uniform float uRoughness;
uniform int uNumSamples = 1024;

// ----------------------------------------------------------------------------

vec3 sample_hemisphere_worldspace( in vec2 pt, in mat3 basis) {
  return normalize(basis * hemisphereSample_cos( pt.x, pt.y ));
}

vec3 sample_rough_hemisphere_worldspace( float roughness_sqr, in vec2 pt, in mat3 basis) {
  pt.x = 1.0 - (1.0 - pt.x) / (1.0 + (roughness_sqr - 1.0) * pt.x);
  return sample_hemisphere_worldspace( pt, basis);
}

// ----------------------------------------------------------------------------

void main() {
  const float inv_samples   = 1.0f / float(uNumSamples);
  const float roughness_sqr = pow(uRoughness, 2.0);

  const vec3 N = normalize(inView);
  const vec3 R = N;
  const vec3 V = R;

  // World-space basis from the view direction / normal.
  const mat3 basis_ws = basis_from_view(N);

  vec3 sum = vec3(0.0);
  float sum_weights = 0.0f;
  for (int i=0; i < uNumSamples; ++i) {
    const vec2 pt = hammersley2d( i, inv_samples);
    const vec3 H  = sample_rough_hemisphere_worldspace( roughness_sqr, pt, basis_ws);
    const vec3 L  = normalize(2.0 * dot(V, H) * H - V);
    
    const float n_dot_l = max(dot(N, L), 0.0);
    if (n_dot_l > 0.0) {
      sum         += texture(uCubemap, L).rgb * n_dot_l;
      sum_weights += n_dot_l;
    }
  }
  sum *= 1.0 / sum_weights;

  fragColor = vec4( sum, 1.0);
}

// ----------------------------------------------------------------------------
