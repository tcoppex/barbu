#ifndef SHADERS_SHARED_LIGHTING_INC_PBR_GLSL_
#define SHADERS_SHARED_LIGHTING_INC_PBR_GLSL_

#include "shared/lighting/inc_common.glsl"
#include "shared/structs/inc_material.glsl"
#include "shared/inc_constants.glsl"          // Pi()

// ----------------------------------------------------------------------------
//
// Specular Microfacet Model.
//
// Using the Cook-Terrance BRDF Model we need three functions :
//  * A Normal Distribution function (ndf) :
//      defines the roughness relative to the half-view angle.
//
//  * A Geometry Function (gf) :
//      defines light obstructions from the view.
//
//  * A Fresnel Function (f) :
//      defines the ratio of light reflected.
//
// ----------------------------------------------------------------------------

// Normal Distribution Function (Trowbridge-Reitz GGX).
float ndf_GGX(in float n_dot_h, in float roughness_sqr) {
  const float d = n_dot_h * n_dot_h * (roughness_sqr - 1.0) + 1.0;
  return roughness_sqr / (d * d * Pi());
}

// ----------------------------------------------------------------------------

// Geometry Function (Schlick GGX).
float gf_SchlickGGX(in float n_dot_v, in float roughness_sqr) {
  const float den = n_dot_v * (1.0 - roughness_sqr) + roughness_sqr;
  return n_dot_v / max(den, Epsilon());
}

// Geometry Function (Smith GGX).
float gf_SmithGGX(in float n_dot_v, in float n_dot_l, in float roughness_sqr) {
  const float ggx_v = gf_SchlickGGX( n_dot_v, roughness_sqr);
  const float ggx_l = gf_SchlickGGX( n_dot_l, roughness_sqr);
  return ggx_v * ggx_l;
}

// ----------------------------------------------------------------------------

// Fresnel Function (Schlick Approximation).
vec3 f_Schlick(in float cosTheta, in vec3 f0) {
  return mix( vec3(pow(1.0 - cosTheta, 5.0)), vec3(1.0), f0);
}

// ----------------------------------------------------------------------------

vec3 brdf_CookTorranceSpecular(in FragLight_t light, in float n_dot_v, in vec3 F, in float roughness_sqr) {
  const float NDF = ndf_GGX( light.n_dot_h, roughness_sqr);
  const float G   = gf_SmithGGX( n_dot_v, light.n_dot_l, roughness_sqr);
  const vec3 num  = NDF * G * F;
  const float den = 4.0f * n_dot_v * light.n_dot_l;
  
  return num / max(den, Epsilon());
}

// ----------------------------------------------------------------------------

struct BRDFMaterial_t {
  vec3 albedo;
  vec3 F0;
  float roughness_sqr;
};

BRDFMaterial_t get_brdf_material(in Material_t mat) {
  // Surface reflection at zero incidence.
  const vec3 kF0 = vec3(0.04);

  const vec3 color = mat.color.rgb;
  
  BRDFMaterial_t brdf_mat;

  // Corrected albedo component of the reflectance equation.
  brdf_mat.albedo = (1.0 - mat.metallic) * (color / Pi());
  // Fresnel parameter.
  brdf_mat.F0 = mix( kF0, color, mat.metallic);
  // Squared roughness.
  brdf_mat.roughness_sqr = pow(mat.roughness, 2);

  return brdf_mat;
}

// ----------------------------------------------------------------------------

vec3 colorize_pbr(in FragInfo_t frag_info, in Material_t mat) {
  //-----------
  LightInfo_t uLightInfos[4];
  int uNumLights = 1;

  LightInfo_t dirlight;
  dirlight.position       = vec4(-5.0, 10.0, 10.0, LIGHT_TYPE_DIRECTIONAL);
  dirlight.direction      = vec4(-normalize(dirlight.position.xyz), 1.0);
  dirlight.color          = vec4(1.0, 1.0, 1.0, 2.0);

  LightInfo_t keylight;
  keylight.position       = vec4(0.0, 0.0, 0.0, LIGHT_TYPE_POINT);
  keylight.direction      = vec4(-normalize(keylight.position.xyz), 1.0);
  keylight.color          = vec4(1.0, 0.0, 0.0, 10.0);

  LightInfo_t filllight;
  filllight.position      = vec4(+4.0, -5.0, +8.0, LIGHT_TYPE_POINT);
  filllight.direction     = vec4(-normalize(filllight.position.xyz), 1.0);
  filllight.color         = vec4(0.0, 1.0, 0.0, 10.0);

  LightInfo_t backlight;
  backlight.position      = vec4(-10.0, 15.0, -6.0, LIGHT_TYPE_POINT);
  backlight.direction     = vec4(-normalize(backlight.position.xyz), 1.0);
  backlight.color         = vec4(0.1, 0.1, 1.0, 50.0);


  uLightInfos[0] = dirlight;
  uLightInfos[1] = keylight;
  uLightInfos[2] = filllight;
  uLightInfos[3] = backlight;
  //-----------

  // Derived the BRDF specific materials from global ones.
  const BRDFMaterial_t brdf_mat = get_brdf_material(mat);

  // Sum reflectance contribution.
  vec3 L0 = vec3(0.0);
  for (int i = 0; i < uNumLights; ++i)
  {
    // Retrieve fragment specific light parameters.
    const FragLight_t light = get_fraglight_params( uLightInfos[i], frag_info );
    
    // Choose between reflection angles.
    const float cosTheta = max(dot( light.H, frag_info.V), 0); // light.n_dot_l

    // Fresnel term.
    const vec3 F = f_Schlick( cosTheta, brdf_mat.F0);
 
    // Deduct the diffuse term from it.
    const vec3 kD = (1.0 - F) * brdf_mat.albedo;

    // Calculate the BRDF specular term.
    const vec3 kS = brdf_CookTorranceSpecular( light, frag_info.n_dot_v, F, brdf_mat.roughness_sqr);

    // Add contribution.
    L0 += (kD + kS) * light.radiance.rgb * light.n_dot_l;

    //L0 = 0.5*light.H + 0.5;
  }

  // Final light color.
  const vec3 color = (L0 + 0.25*mat.ambient) * mat.ao;

  return color;
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_LIGHTING_INC_PBR_GLSL_
