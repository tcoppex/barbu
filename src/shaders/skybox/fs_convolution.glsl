#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------
//
// Calculate the convolution of an environment map.
//
// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inView;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform samplerCube uCubemap;
uniform int uNumLongSamples = 200;

// ----------------------------------------------------------------------------

void main() {
  vec3 irradiance = vec3(0.0);
  
  // World-space basis from the view direction.
  const mat3 basis_ws = basis_from_view(inView);

  // --------------

  // Number of longitudinal samples (in 2 * Pi).
  const int kNumLongSamples = uNumLongSamples;

  // Number of lateral samples (in Pi / 2).
  const int kNumLatSamples  = int(ceil(kNumLongSamples / 4.0));

  // Steps angles, based off longitudinals samples count.
  const float step_radians = TwoPi() / kNumLongSamples;

  // Step angles in trigonometric form.
  const vec2 delta = trig_angle(step_radians);

  // Start angle in trigonometric form.
  const vec2 start_angle = trig_angle(0.0);

  // Convolution kernel.
  vec2 phi = start_angle;
  for (int y=0; y < kNumLongSamples; ++y) {
    vec2 theta = start_angle;
    for (int x=0; x < kNumLatSamples; ++x) {

      // Transform polar coordinates to view ray from camera.
      const vec3 ray = vec3(phi, 1.0) * theta.yyx; 

      // Transform view ray to world space.
      const vec3 ray_ws = basis_ws * ray;

      // Add weighted environment sample to total irradiance.
      irradiance += texture( uCubemap, ray_ws, 0).rgb * theta.x * theta.y;

      theta = inc_trig_angle( theta, delta);
    }
    phi = inc_trig_angle( phi, delta);
  }

  // Weight total irradiance.
  irradiance *= Pi() / float(kNumLongSamples * kNumLatSamples);

  // --------------

  fragColor = vec4(irradiance, 1.0);
}

// ----------------------------------------------------------------------------
