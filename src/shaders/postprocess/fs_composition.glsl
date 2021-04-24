#version 430 core

#include "shared/inc_tonemapping.glsl"

// ----------------------------------------------------------------------------

in layout(location = 0) vec2 vTexCoord;

out layout(location = 0) vec4 fragColor;

uniform layout(location = 0) sampler2D uColor;
uniform layout(location = 1) sampler2D uAO;
uniform int uToneMapMode = TONEMAPPING_ROMBINDAHOUSE; //

// ----------------------------------------------------------------------------

void main() {
  vec3 rgb = texture(uColor, vTexCoord).rgb;
  
  // Ambient Occlusion.
  const float ao = texture(uAO, vTexCoord).r;
  rgb *= vec3(ao);

  // Tone Mapping.
#if 0
  // (display different methods)
  const float dx = gl_FragCoord.x / float(textureSize(uColor, 0).x);
  rgb = testingToneMapping(rgb, dx);  
#else
  rgb = toneMapping(uToneMapMode, rgb);
#endif

  fragColor = vec4( rgb, 1.0f);
}

// ----------------------------------------------------------------------------