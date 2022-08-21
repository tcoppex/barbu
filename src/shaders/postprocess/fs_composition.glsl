#version 430 core

#include "shared/inc_tonemapping.glsl"

// ----------------------------------------------------------------------------

in layout(location = 0) vec2 vTexCoord;

out layout(location = 0) vec4 fragColor;

uniform layout(location = 0) sampler2D uAlbedo;
uniform layout(location = 1) sampler2D uEmissive;
uniform layout(location = 2) sampler2D uAO;
uniform int uToneMapMode = TONEMAPPING_ROMBINDAHOUSE; //

// ----------------------------------------------------------------------------

void main() {
  vec3 rgb = texture(uAlbedo, vTexCoord).rgb;

  // Emissive.
  vec4 emissive = texture(uEmissive, vTexCoord).rgba;
  float emissiveFactor = 2.0;
  // emissive.rgb = smoothstep(vec3(0.025), vec3(0.75), pow(emissive.rgb, vec3(0.75)));
  rgb += emissiveFactor * emissive.rgb;

  // Ambient Occlusion.
  const float ao = texture(uAO, vTexCoord).r;
  rgb *= vec3(ao);

  // Tone Mapping.
#if 1
  rgb = toneMapping(uToneMapMode, rgb);
#else
  // [Debug] Display different methods.
  const float dx = gl_FragCoord.x / float(textureSize(uAlbedo, 0).x);
  rgb = testingToneMapping(rgb, dx);  
#endif

  fragColor = vec4(rgb, 1.0f);
}

// ----------------------------------------------------------------------------