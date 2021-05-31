#version 430 core

#include "generic/interop.h"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec3 inPositionWS;
layout(location = 2) in vec3 inNormalWS;
layout(location = 3) in vec4 inTangentWS;

// Outputs.
layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

#include "shared/lighting/inc_pbr.glsl"
#include "shared/structs/inc_fraginfo.glsl"
#include "shared/structs/inc_material.glsl"
#include "shared/inc_tonemapping.glsl"
#include "shared/inc_maths.glsl"

// Uniforms : Default Material.
uniform sampler2D uBRDFMap;
uniform samplerCube uPrefilterEnvmap;
uniform samplerCube uIrradianceEnvmap;
uniform mat4 uIrradianceMatrices[3];
uniform bool uHasIrradianceMatrices;
uniform vec3 uEyePosWS;
uniform int uToneMapMode = TONEMAPPING_NONE;

// Uniforms : Generic Material.
uniform int uColorMode;
uniform vec4 uColor;
uniform float uAlphaCutOff;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uEmissiveFactor;

uniform sampler2D uAlbedoTex;
uniform sampler2D uNormalTex;
uniform sampler2D uRoughMetalTex;
uniform sampler2D uAOTex;
uniform sampler2D uEmissiveTex;

uniform bool uHasAlbedo;
uniform bool uHasNormal;
uniform bool uHasRoughMetal;
uniform bool uHasAO;
uniform bool uHasEmissive;

// ----------------------------------------------------------------------------

vec3 get_normal() {
  vec3 N = normalize(inNormalWS);

  // Retrieve the bump normal in tangent space using mikkTSpace decoding.
  if (uHasNormal) {
    // TBN basis to transform from tangent-space to world-space.
    // We do not have to normalize this interpolated vectors to get the TBN.
    const float s = 1.0; //inTangentWS.w;
    const vec3 T = inTangentWS.xyz;
    const vec3 B = s * cross( N, T);
    const mat3 TBN = mat3( T, B, N);
    
    // Tangent-space normal.
    vec3 Nt = texture( uNormalTex, inTexcoord.xy).xyz;
    Nt = gamma_uncorrect(Nt); //

    // World-space bump normal.
    const vec3 bump = normalize(TBN * Nt);

    // Contribute to the main normal.
    //N = normalize(mix(N, bump, 5.5)); //
    N = bump;
  }

  return N;
}

// Extract fragment geometry infos.
FragInfo_t get_worldspace_fraginfo() {
  FragInfo_t frag;
  
  frag.P        = inPositionWS;
  frag.N        = get_normal();
  frag.V        = normalize( uEyePosWS - frag.P );
  frag.R        = reflect( -frag.V, frag.N);
  frag.uv       = inTexcoord.xy;
  frag.n_dot_v  = saturate(dot(frag.N, frag.V)); //

  // [fixme] Deal with double sided plane.
  // frag.N *= sign(dot(frag.N, frag.V));

  return frag;
}

// ----------------------------------------------------------------------------

vec3 get_irradiance(in vec3 normal_ws) {
  vec3 irradiance = vec3(0.0);

  if (uHasIrradianceMatrices) {
    const vec4 n = vec4( normal_ws, 1.0);
    irradiance = vec3(
      dot( n, uIrradianceMatrices[0] * n),
      dot( n, uIrradianceMatrices[1] * n),
      dot( n, uIrradianceMatrices[2] * n)
    );
  } else {
    const float kIrradianceMapFactor = 0.5; //
    irradiance = kIrradianceMapFactor * texture( uIrradianceEnvmap, normal_ws).rgb;
  }

  return irradiance;
}

// ----------------------------------------------------------------------------

// Extract material infos, also test for alpha coverage.
Material_t get_material(in FragInfo_t frag) {
  Material_t mat;

  // Diffuse / Albedo.
  mat.color = (uHasAlbedo) ? texture( uAlbedoTex, frag.uv) : uColor;

  // Early Alpha fails.
  if (mat.color.a <= uAlphaCutOff) { 
    discard; 
  }
  
  // Roughness + Metallic.
  const vec2 rough_metal = (uHasRoughMetal) ? texture( uRoughMetalTex, frag.uv).yz // 
                                            : vec2( uRoughness, uMetallic );
  mat.roughness = max(rough_metal.x, 0.001);
  mat.metallic  = rough_metal.y;

  // Ambient Occlusion.
  const float ao = (uHasAO) ? texture( uAOTex, frag.uv).r : 1.0f;
  mat.ao = pow(ao, 1.5);

  // Emissive.
  const vec3 emissive = (uHasEmissive) ? texture(uEmissiveTex, frag.uv).rgb : vec3(0.0f);
  mat.emissive = emissive * uEmissiveFactor;

  // -- fragment derivative materials ---
  {
    // Diffuse irradiance.
    mat.irradiance = get_irradiance( frag.N );

    // Roughness based prefiltered specular.
    const float roughness_level = mat.roughness * log(textureSize(uPrefilterEnvmap, 0).x);
    mat.prefiltered = textureLod( uPrefilterEnvmap, frag.R, roughness_level).rgb;

    // Roughness based BRDF specular values. [weird issue on edges].
    const vec2 brdf_uv = vec2( frag.n_dot_v, mat.roughness);
    mat.BRDF = texture( uBRDFMap, brdf_uv).rg;
  }

  return mat;
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
