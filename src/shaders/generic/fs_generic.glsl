#version 430 core

#include "generic/interop.h"

#include "shared/lighting/inc_pbr.glsl"

#include "shared/inc_tonemapping.glsl"

// #include "shared/structs/inc_fraginfo.glsl"
// #include "shared/structs/inc_material.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPositionWS;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormalWS;
layout(location = 3) in vec3 inIrradiance;

// Outputs.
layout(location = 0) out vec4 fragColor;

// Uniforms : Generic.
uniform int uToneMapMode = TONEMAPPING_NONE;
uniform vec3 uEyePosWS;

// Uniforms : Material.
uniform int uColorMode;
uniform vec4 uColor;
uniform float uAlphaCutOff;
uniform float uMetallic  = 0.0f;
uniform float uRoughness = 0.5f;

uniform sampler2D uAlbedoTex;
uniform sampler2D uMetalRoughTex;

uniform bool uHasAlbedo;
uniform bool uHasMetalRough = false;

// ----------------------------------------------------------------------------

// Extract material infos, also test for alpha coverage.
Material_t get_material() {
  Material_t mat;

  const vec2 uv = inTexcoord.xy;

  // Diffuse / Albedo.
  mat.color = (uHasAlbedo) ? texture( uAlbedoTex, uv) : uColor;

  // Early Alpha fails.
  if (mat.color.a < uAlphaCutOff) { discard; }
  
  // Metallic / Roughness
  const vec2 metallic_roughness = (uHasMetalRough) ? texture( uMetalRoughTex, uv).rg 
                                                   : vec2(uMetallic, uRoughness)
                                                   ; 
  mat.metallic = metallic_roughness.x;
  mat.roughness = metallic_roughness.y;

  // Environment map Irradiance (from the Vertex Shader).
  mat.irradiance = inIrradiance;

  return mat;
}

// Extract fragment geometry infos.
FragInfo_t get_worldspace_fraginfo() {
  FragInfo_t frag;
  frag.P        = inPositionWS;
  frag.N        = normalize( inNormalWS );
  frag.V        = normalize( frag.P - uEyePosWS );
  frag.uv       = inTexcoord.xy;
  frag.n_dot_v  = max( dot(frag.N, frag.V), 0 );
  return frag;
}

// ----------------------------------------------------------------------------

#if 0

vec3 test_pbr(in FragInfo_t frag, in Material_t mat) {
  // Create a Key / Fill / Back lighting setup.
  LightInfo_t keylight;
  keylight.position.xyz   = vec3(0.0, 0.0, 0.0);
  keylight.color          = vec4(1.0, 0.0, 0.0, 1.0);

  LightInfo_t filllight;
  filllight.position.xyz  = vec3(+4.0, -5.0, +8.0);
  filllight.color         = vec4(0.0, 1.0, 0.0, 1.0);

  LightInfo_t backlight;
  backlight.position.xyz  = vec3(-10.0, 15.0, -6.0);
  backlight.color         = vec4(0.1, 0.1, 1.0, 1.0);

  const float kLightRadius = 50.0f;

  vec3 light = 
   + apply_point_light( keylight, frag.P, frag.N, frag.V, kLightRadius)
   + apply_point_light( filllight, frag.P, frag.N, frag.V, kLightRadius)
   + apply_point_light( backlight, frag.P, frag.N, frag.V, kLightRadius)
  ;

  return mat.color.rgb * mix(inIrradiance, light, 0.98);
}

#endif

// ----------------------------------------------------------------------------

vec4 colorize(in int color_mode, in FragInfo_t frag, in Material_t mat) {
  vec3 rgb;

  switch (color_mode) {

    case MATERIAL_GENERIC_COLOR_MODE_LIGHT_PBR:
      // todo
    break;

    case MATERIAL_GENERIC_COLOR_MODE_NORMAL:
      rgb = vec3( 0.5 * (frag.N + 1.0) );
      rgb = gamma_uncorrect(rgb);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_TEXCOORD:
      rgb = vec3( frag.uv, 1.0);
      rgb = gamma_uncorrect(rgb);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_IRRADIANCE:
      rgb = mat.color.rgb * inIrradiance;
    break;
    
    default:
    case MATERIAL_GENERIC_COLOR_MODE_UNLIT:
      rgb = mat.color.rgb;
    break;
  }

  // Tonemapping is generally done in the postprocess stage, however for forward
  // passes - like blending - it should be used directly. 
  rgb = toneMapping( uToneMapMode, rgb);

  return vec4(rgb, mat.color.a);
}

// ----------------------------------------------------------------------------

void main() {
  const Material_t material = get_material();
  const FragInfo_t fraginfo = get_worldspace_fraginfo();
  fragColor = colorize( uColorMode, fraginfo, material);

  // fragColor.rgb = test_pbr( fraginfo, material);
}

// ----------------------------------------------------------------------------
