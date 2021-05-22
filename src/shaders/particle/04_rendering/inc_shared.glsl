#ifndef SHADERS_PARTICLE_RENDERING_SHARED_GLSL_
#define SHADERS_PARTICLE_RENDERING_SHARED_GLSL_

#include "shared/inc_tonemapping.glsl" // [tmp]

uniform float uFadeCoefficient = 0.25f;
//uniform sampler2D uSpriteSampler2d;

vec4 compute_color(in vec3 base_color, in float decay, in vec2 texcoord) {
  vec4 color = vec4(base_color, 1.0f);

  // Centered coordinates.
  const vec2 p = 2.0f * (texcoord - 0.5f);
  
  // Pixel intensity depends on its distance from center.
  const float d = 1.0f - abs(dot(p, p));

  // Alpha coefficient.
  float alpha = smoothstep(0.0f, 1.0f, d) * decay * uFadeCoefficient;

  //color = texture(uSpriteSampler2d, texcoord).rrrr;
  //color *= alpha;

  color.rgb = toneMapping( TONEMAPPING_ROMBINDAHOUSE, color.rgb); //
  color.a = alpha;

  return color;
}

#endif // SHADERS_PARTICLE_RENDERING_SHARED_GLSL_