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
uniform float uMetallic;
uniform float uRoughness;

uniform sampler2D uAlbedoTex;
uniform sampler2D uMetalRoughTex;
uniform sampler2D uAOTex;

uniform bool uHasAlbedo;
uniform bool uHasMetalRough;
uniform bool uHasAO;

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
  
  // Ambient Occlusion.
  const float ao = (uHasAO) ? texture( uAOTex, uv).r : 1.0f;

  // Metallic + Roughness.
  const vec2 metal_rough = (uHasMetalRough) ? texture( uMetalRoughTex, uv).yz 
                                            : vec2( uMetallic, uRoughness );
  mat.metallic  = metal_rough.x;
  mat.roughness = metal_rough.y;

  // Ambient using environment map Irradiance from the Vertex Shader.
  mat.ambient = mat.color.rgb * inIrradiance;
  mat.ao = pow(ao, 1.0);

  return mat;
}

// Extract fragment geometry infos.
FragInfo_t get_worldspace_fraginfo() {
  FragInfo_t frag;
  
  frag.P        = inPositionWS;
  frag.N        = normalize( inNormalWS );
  frag.V        = normalize( uEyePosWS - frag.P );
  frag.uv       = inTexcoord.xy;
  frag.n_dot_v  = dot(frag.N, frag.V);

  // Deal with double sided plane.
  const float s = sign(frag.n_dot_v);
  frag.N *= s;

  return frag;
}

// ----------------------------------------------------------------------------

vec4 colorize(in int color_mode, in FragInfo_t frag, in Material_t mat) {
  vec3 rgb;

  switch (color_mode) {
    default:
    case MATERIAL_GENERIC_COLOR_MODE_PBR:
      rgb = colorize_pbr( frag, mat); 
    break;

    case MATERIAL_GENERIC_COLOR_MODE_UNLIT:
      rgb = mat.color.rgb;
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
    
    case MATERIAL_GENERIC_COLOR_MODE_AO:
      rgb = vec3(mat.ao);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_METALLIC:
      rgb = vec3(mat.metallic);
      rgb = gamma_uncorrect(rgb);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_ROUGHNESS:
      rgb = vec3(mat.roughness);
      rgb = gamma_uncorrect(rgb);
    break;
  }

  // Tonemapping is generally done in the postprocess stage, however for forward
  // passes - like blending - it should be applied directly. 
  //rgb = toneMapping( uToneMapMode, rgb);

  return vec4(rgb, mat.color.a);
}

// ----------------------------------------------------------------------------

void main() {
  /*const*/ Material_t material = get_material();
  /*const*/ FragInfo_t fraginfo = get_worldspace_fraginfo();
  fragColor = colorize( uColorMode, fraginfo, material);
}

// ----------------------------------------------------------------------------
