#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------
//
// Calculate the specular prefiltered convolution of an environment map.
//
// Increasing level of the cubemap correspond to a higher rough reflection level. 
//
// Ref for optimizations : 
//  * https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
//  * https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
//  * https://www.tobias-franke.eu/log/2014/03/30/notes_on_importance_sampling.html
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

void main() {
  const float inv_samples   = 1.0f / float(uNumSamples);
  const float roughness_sqr = pow(uRoughness, 2.0);

  const vec3 N = normalize(inView);
  const vec3 R = N;
  const vec3 V = R;

  // World-space basis from the view direction / normal.
  const mat3 basis_ws = basis_from_view(N);

  vec3 sum = vec3(0.0);
  float weights = 0.0f;

  for (int i=0; i < uNumSamples; ++i) {
    const vec2 pt = hammersley2d( i, inv_samples);
    const vec3 H  = importance_sample_GGX( basis_ws, pt, roughness_sqr);
    const vec3 L  = normalize(2.0 * dot(V, H) * H - V);
    
    const float n_dot_l = max(dot(N, L), 0.0);
    if (n_dot_l > 0.0) {
      sum     += texture(uCubemap, L).rgb * n_dot_l;
      weights += n_dot_l;
    }
  }
  sum *= 1.0 / weights;

  fragColor = vec4( sum, 1.0);
}

// ----------------------------------------------------------------------------
