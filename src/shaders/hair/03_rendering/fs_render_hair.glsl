#version 430 core

#include "shared/inc_maths.glsl"
#include "shared/inc_lighting.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;

// Outputs.
layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

// Marschner Reflectance LUTs.
uniform sampler2D uLongitudinalLUT;       // RGBA : MR, MTT, MTRT, cosThetaD
uniform sampler2D uAzimuthalLUT;          // RGB  : NR, NTT, NTRT
uniform vec3 uAlbedo;

// ----------------------------------------------------------------------------

vec3 reflectance_ray_polarcoords(
  in vec3 light_dir, 
  in vec3 look_dir, 
  in vec3 tangent) 
{
  const float sinThetaI = dot( light_dir, tangent);
  const float sinThetaO = dot( look_dir, tangent);

  // Calculate the light and eye direction perpendicular to tangent.
  const vec3 light_perp = light_dir - sinThetaI * tangent;
  const vec3 look_perp  = look_dir - sinThetaO * tangent;

  const float cosPhiD = dot(light_perp, look_perp)
                      * inversesqrt(dot(light_perp, light_perp) * dot(look_perp, look_perp))
                      ;

  return vec3(sinThetaI, sinThetaO, cosPhiD);
}

vec2 polarcoords_to_uv(in vec2 coords, float texres) {
  return vec2( 
    maprange(-1, 1, coords.x), 
    maprange(-1, 1, coords.y)
  );
}

float marschner_reflectance(in vec3 light_dir, in vec3 look_dir, in vec3 tangent) {
  const float textureRes = float(textureSize(uLongitudinalLUT, 0));

  // Retrieve the light ray polar coordinates.
  const vec3 coords = reflectance_ray_polarcoords(light_dir, look_dir, tangent);

  // Retrieve the longitudinal LUTs (+ cosThetaD).
  vec2 st = polarcoords_to_uv( coords.xy, textureRes );
  const vec4 M = texture( uLongitudinalLUT, st).rgba;
  const float cosThetaD = M.w;

  // Retrieve the azymuthal LUTs.
  st = polarcoords_to_uv( vec2(coords.z, cosThetaD), textureRes );
  const vec3 N = texture( uAzimuthalLUT, st).rgb;

  const float S = dot(M.xyz, N.xyz) / max(pow(cosThetaD, 2.0), Epsilon());

  return S;
}

// ----------------------------------------------------------------------------

void main() {
  const vec3 albedo = uAlbedo;
  
  //-------

  // Ambient.
  const float ambient_factor = 0.1; //

  // Marschner Reflectance.
  const vec3 light_dir = normalize(vec3(0.0, 0.0, 1.0));
  const vec3 eye_dir   = normalize(inPosition.xyz);
  const vec3 tangent   = normalize(inTangent);

  const float rfactor = marschner_reflectance( light_dir, eye_dir, tangent);
  const float reflectance = pow(rfactor, 0.125); //

  // Diffuse light
  vec3 diffuse;
  {
    Light light;
    light.direction.xyz  = light_dir;
    light.color          = vec4(1.0, 1.0, 1.0, 1.0);
    diffuse = apply_directional_light( light, inNormal) * albedo;
  }

  //-------

  const vec3 lighting = ambient_factor + reflectance * diffuse;

  fragColor.rgb = lighting * albedo;
  fragColor.a = 1.0;
}

// ----------------------------------------------------------------------------
