#version 430 core

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

// Inputs.
layout(location = 0) in vec3 inPosition;
//layout(location = 1) in float inColorCoeff;

// Outputs.
layout(location = 0) out vec4 fragColor;

// ----------------------------------------------------------------------------

// Marschner Reflectance LUTs.
uniform sampler2D uLongitudinalLUT;       // RGBA : MR, MTT, MTRT, cosThetaD
uniform sampler2D uAzimuthalLUT;          // RGB  : NR, NTT, NTRT

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
  const vec3 look_perp  = light_dir - sinThetaO * tangent;

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

  const float S = dot(M.xyz, N.xyz) / max(pow(cosThetaD, 2), Epsilon());

  return 99*abs(S);
}

// ----------------------------------------------------------------------------

void main() {
  fragColor.rgb = /*inColorCoeff * */vec3(0.218, 0.16, 0.142);
  
  // [test]
  fragColor.rgb *= marschner_reflectance(
    normalize(vec3(0., 0., 1.)),
    normalize(inPosition.xyz),
    normalize(vec3(0.0, -1, 0.0))
  );

  fragColor.a = 1.0;
}

// ----------------------------------------------------------------------------
