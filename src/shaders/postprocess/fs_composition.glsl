#version 430 core

#include "postprocess/inc_tonemapping.glsl"

// ----------------------------------------------------------------------------

in layout(location = 0) vec2 vTexCoord;

out layout(location = 0) vec4 fragColor;

uniform layout(location = 0) sampler2D uColor;
uniform layout(location = 1) sampler2D uAO;

uniform int uToneMapMode = 4;//TONEMAPPING_ROMBINDAHOUSE;

// ----------------------------------------------------------------------------

void main() {
  vec3 rgb = texture(uColor, vTexCoord).rgb;
  float ao = texture(uAO, vTexCoord).r;

  rgb *= vec3(ao);

  // (display different methods)
  // float dx = gl_FragCoord.x / float(textureSize(uColor, 0).x);
  // rgb = testingToneMapping(rgb, dx);  

  rgb = toneMapping(rgb, uToneMapMode);

  fragColor = vec4( rgb, 1.0f);
}

// ----------------------------------------------------------------------------