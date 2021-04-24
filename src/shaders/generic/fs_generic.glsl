#version 430 core

#include "generic/interop.h"

#include "shared/lighting/inc_pbr.glsl"
#include "shared/structs/inc_fraginfo.glsl"
#include "shared/structs/inc_material.glsl"
#include "shared/inc_tonemapping.glsl"

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
uniform float uMetallic  = 0.70f;
uniform float uRoughness = 0.15f; //

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
  if (mat.color.a < uAlphaCutOff) { 
    discard; 
  }
  
  // Metallic + Roughness.
  const vec2 metallic_roughness = (uHasMetalRough) ? texture( uMetalRoughTex, uv).rg 
                                                   : vec2(uMetallic, uRoughness); 
  mat.metallic  = metallic_roughness.x;
  mat.roughness = metallic_roughness.y;

  // Ambient using environment map Irradiance from the Vertex Shader.
  mat.ambient = mat.color.rgb * inIrradiance;

  return mat;
}

// Extract fragment geometry infos.
FragInfo_t get_worldspace_fraginfo() {
  FragInfo_t frag;
  frag.P        = inPositionWS;
  frag.N        = normalize( inNormalWS );
  frag.V        = normalize( frag.P - (-uEyePosWS) );               // XXXXXX
  frag.uv       = inTexcoord.xy;
  frag.n_dot_v  = max( dot(frag.N, frag.V), 0 );
  return frag;
}

// ----------------------------------------------------------------------------

vec4 colorize(in int color_mode, in FragInfo_t frag, in Material_t mat) {
  vec3 rgb;

  switch (color_mode) {

    case MATERIAL_GENERIC_COLOR_MODE_LIGHT_PBR:
      rgb = colorize_pbr( frag, mat); 
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
      rgb = mat.ambient;
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
}

// ----------------------------------------------------------------------------
