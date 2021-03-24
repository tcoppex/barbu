#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

layout(location=0) in vec3 position;
layout(location=1) in vec3 velocity;
layout(location=2) in vec2 age_info;

uniform mat4 uMVP;
uniform float uMinParticleSize = 1.0f; //
uniform float uMaxParticleSize = 6.0f; //
uniform uint uColorMode = 0;
uniform vec3 uBirthGradient = vec3(1.0f, 0.0f, 0.0f);
uniform vec3 uDeathGradient = vec3(0.0f);

out VDataBlock {
  vec3 position;
  vec3 velocity;
  vec3 color;
  float decay;
  float pointSize;
} OUT;

// ----------------------------------------------------------------------------

float compute_size(float z, float decay) {
  const float min_size = uMinParticleSize;
  const float max_size = uMaxParticleSize;

  // Scale the size relative to camera distance, set it to 1 to have normal size.
  const float depth = (max_size-min_size) / (z);

  float x = decay * depth;
  
  float size = mix(min_size, max_size, x);

  // size grow quadratically, we might want to relate to sprite area instead.
  //size = sqrt(size / 3.14156492);

  return size;
}

// ----------------------------------------------------------------------------

vec3 base_color(in vec3 position, in float decay) {
  // Gradient mode
  if (uColorMode == 1) {
    return mix(uBirthGradient, uDeathGradient, decay);
  }
  // Default mode
  return 0.5f * (normalize(position) + 1.0f);
}

// ----------------------------------------------------------------------------

void main() {
  const vec3 p = position.xyz;

  // Time alived in [0, 1].
  const float dAge = 1.0f - maprange(0.0f, age_info.x, age_info.y);
  const float decay = smoothcurve( 0.5f, dAge);

  // Vertex attributes.
  gl_Position = uMVP * vec4(p, 1.0f);
  gl_PointSize = compute_size(gl_Position.z, decay);

  // Output parameters.
  OUT.position = p;
  OUT.velocity = velocity.xyz;
  OUT.color = base_color(position, decay);
  OUT.decay = decay;
  OUT.pointSize = gl_PointSize;
}

// ----------------------------------------------------------------------------
