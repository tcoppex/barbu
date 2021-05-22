#version 430 core

#include "generic/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPositionWS;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormalWS;

// Outputs.
layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

#include "shared/lighting/inc_pbr.glsl"
#include "shared/structs/inc_fraginfo.glsl"
#include "shared/structs/inc_material.glsl"
#include "shared/inc_tonemapping.glsl"

// Uniforms : Default Material.
uniform vec3 uEyePosWS;
uniform samplerCube uEnvironmentMap;
uniform samplerCube uIrradianceMap;
uniform mat4 uIrradianceMatrices[3];
uniform bool uHasIrradianceMatrices;
uniform int uToneMapMode = TONEMAPPING_NONE;

// Uniforms : Generic Material.
uniform int uColorMode;
uniform vec4 uColor;
uniform float uAlphaCutOff;
uniform float uMetallic;
uniform float uRoughness;

uniform sampler2D uAlbedoTex;
uniform sampler2D uRoughMetalTex;
uniform sampler2D uAOTex;

uniform bool uHasAlbedo;
uniform bool uHasRoughMetal;
uniform bool uHasAO;

// ----------------------------------------------------------------------------

vec3 get_irradiance(in vec3 normalWS) {
  vec3 irradiance = vec3(0.0);

  if (uHasIrradianceMatrices) {
    const vec4 n = vec4( normalWS, 1.0);
    irradiance = vec3(
      dot( n, uIrradianceMatrices[0] * n),
      dot( n, uIrradianceMatrices[1] * n),
      dot( n, uIrradianceMatrices[2] * n)
    );
  } else {
    const float irr_factor = 0.5; //
    irradiance = irr_factor * texture( uIrradianceMap, normalWS).rgb;
  }

  return irradiance;
}

// ----------------------------------------------------------------------------

// Extract material infos, also test for alpha coverage.
Material_t get_material(in FragInfo_t frag) {
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

  // Roughness + Metallic.
  const vec2 rough_metal = (uHasRoughMetal) ? texture( uRoughMetalTex, uv).yz // 
                                            : vec2( uRoughness, uMetallic );
  mat.roughness = rough_metal.x;
  mat.metallic  = rough_metal.y;

  mat.ao = pow(ao, 2.0);

  // --------

  // [ fragment derivative materials ]

  // Compose ambient with a fragment based irradiance color.
  mat.irradiance = get_irradiance( frag.N );
  mat.reflection = texture( uEnvironmentMap, frag.R).rgb; //

  return mat;
}

// Extract fragment geometry infos.
FragInfo_t get_worldspace_fraginfo() {
  FragInfo_t frag;
  
  frag.P        = inPositionWS;
  frag.N        = normalize( inNormalWS );
  frag.V        = normalize( uEyePosWS - frag.P );
  frag.R        = reflect( -frag.V, frag.N);
  frag.uv       = inTexcoord.xy;
  frag.n_dot_v  = max(dot(frag.N, frag.V), 0);

  // [fixme] Deal with double sided plane.
  // const float s = sign(frag.n_dot_v);
  // frag.N *= s;

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
      rgb = mat.irradiance;
    break;
    
    case MATERIAL_GENERIC_COLOR_MODE_AO:
      rgb = vec3(mat.ao);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_ROUGHNESS:
      rgb = vec3(mat.roughness);
      rgb = gamma_uncorrect(rgb);
    break;

    case MATERIAL_GENERIC_COLOR_MODE_METALLIC:
      rgb = vec3(mat.metallic);
      rgb = gamma_uncorrect(rgb);
    break;
  }

  // Tonemapping is generally done in the postprocess stage, however for forward
  // passes - like blending - it should be applied directly. 
  rgb = toneMapping( uToneMapMode, rgb);

  return vec4(rgb, mat.color.a);
}

// ----------------------------------------------------------------------------

void main() {
  FragInfo_t fraginfo = get_worldspace_fraginfo();
  Material_t material = get_material(fraginfo);

  fragColor = colorize( uColorMode, fraginfo, material);
}

// ----------------------------------------------------------------------------
