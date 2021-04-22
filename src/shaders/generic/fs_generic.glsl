#version 430 core

#include "shared/inc_lighting.glsl"

// ----------------------------------------------------------------------------

#define CM_UNLIT        0
#define CM_NORMALS      1
#define CM_KEY_LIGHTS   2
#define CM_IRRADIANCE   3
#define CM_TEXCOORDS    4

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPositionWS;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormalWS;
layout(location = 3) in vec3 inViewDirWS;
layout(location = 4) in vec3 inIrradiance;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms.
uniform sampler2D uAlbedoTex;
uniform vec4 uColor        = vec4(1.0f);
uniform bool uHasAlbedo    = true;
uniform int uColorMode     = CM_IRRADIANCE;
uniform float uAlphaCutOff = 0.5f;

// ----------------------------------------------------------------------------

vec3 color_keylights(in vec3 rgb) {
  // Create a Key / Fill / Back lighting setup.
  Light keylight;
  keylight.direction.xyz  = vec3(-1.0, -1.0, -1.0);
  keylight.color          = vec4(1.0, 1.0, 0.95, 0.5);

  Light filllight;
  filllight.direction.xyz = vec3(+4.0, -1.0, +4.0);
  filllight.color         = vec4(0.5, 0.75, 0.2, 0.95);

  Light backlight;
  backlight.position.xyz  = vec3(-10.0, 15.0, -12.0);
  backlight.color         = vec4(0.4, 0.4, 1.0, 0.8);

  vec3 diffuse = apply_directional_light(keylight, inNormalWS)
               + apply_directional_light(filllight, inNormalWS)
               + apply_point_light(backlight, inPositionWS, inNormalWS)
               ;

  const vec3 ambient = vec3(0.1); //
  diffuse = clamp(diffuse, vec3(0), vec3(1));

  return rgb * (ambient + diffuse);
}

vec3 color_normal(in vec3 rgb) {
  rgb = 0.5 * (inNormalWS + 1.0);
  // (compensate for gamma correction)
  return pow(rgb, vec3(2.2)); //
}

vec3 color_irradiance(in vec3 rgb) {
  return rgb * inIrradiance;
}

vec3 color_texcoords(in vec3 rgb) {
  return vec3( 0.5 + 0.5*inTexcoord, 0.5);
}

vec3 colorize(in int color_mode, in vec3 rgb) {
  // (might use a subroutine instead)
  if (color_mode == CM_KEY_LIGHTS) {
    rgb = color_keylights(rgb);
  } else if (color_mode == CM_NORMALS) {
    rgb = color_normal(rgb);
  } else if (color_mode == CM_IRRADIANCE) {
    rgb = color_irradiance(rgb);
  } else if (color_mode == CM_TEXCOORDS) {
    rgb = color_texcoords(rgb);
  }

  return rgb;
}

// ----------------------------------------------------------------------------

void main() {
  vec4 color = vec4(1.0);

  if (uHasAlbedo) {
    color = texture(uAlbedoTex, inTexcoord.xy);
  } else {
    color = uColor;
  }

  if (color.a < uAlphaCutOff) { 
    discard;
  }

  fragColor.xyz = colorize( uColorMode, color.rgb);
  fragColor.a   = color.a;
}

// ----------------------------------------------------------------------------
