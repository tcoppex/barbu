#ifndef SHADERS_SHARED_INC_TONEMAPPING_GLSL_
#define SHADERS_SHARED_INC_TONEMAPPING_GLSL_

// ----------------------------------------------------------------------------
/*
  Tonemapping operators.

  Adapted from https://github.com/worleydl/hdr-shaders

  References :
    https://gdcvault.com/play/1012351/Uncharted-2-HDR
    http://slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting
    http://filmicworlds.com/blog/filmic-tonemapping-operators/
    http://filmicworlds.com/blog/why-reinhard-desaturates-your-blacks/
    http://filmicworlds.com/blog/why-a-filmic-curve-saturates-your-blacks/
    http://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3/
    http://mynameismjp.wordpress.com/2010/04/30/a-closer-look-at-tone-mapping/
    http://renderwonk.com/publications/s2010-color-course/
*/
// ----------------------------------------------------------------------------

#include "shared/inc_gamma.glsl"

// ----------------------------------------------------------------------------

#define TONEMAPPING_NONE              -1
#define TONEMAPPING_LINEAR            0
#define TONEMAPPING_SIMPLE_REINHARD   1
#define TONEMAPPING_LUMA_REINHARD     2
#define TONEMAPPING_LUMA_REINHARD_2   3
#define TONEMAPPING_ROMBINDAHOUSE     4
#define TONEMAPPING_FILMIC            5
#define TONEMAPPING_UNCHARTED         6

// ----------------------------------------------------------------------------

vec3 linearToneMapping(vec3 color) {
  const float exposure = 1.0;
  color = clamp(exposure * color, 0.0, 1.0);
  color = gamma_correct(color);
  return color;
}

vec3 simpleReinhardToneMapping(vec3 color) {
  const float exposure = 1.0;
  color *= exposure / (1.0 + color / exposure);
  color = gamma_correct(color);
  return color;
}

vec3 lumaBasedReinhardToneMapping(vec3 color) {
  float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
  float toneMappedLuma = luma / (1.0 + luma);
  color *= toneMappedLuma / luma;
  color = gamma_correct(color);
  return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color) {
  const float white = 2.0;
  const float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
  const float toneMappedLuma = luma * (1.0 + luma / (white*white)) / (1.0 + luma);
  color *= toneMappedLuma / luma;
  color = gamma_correct(color);
  return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color) {
  color = exp( -1.0 / ( 2.72*color + 0.15 ) );
  color = gamma_correct(color);
  return color;
}

vec3 filmicToneMapping(vec3 color) {
  color = max(vec3(0.0), color - vec3(0.004));
  color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
  return color;
}

vec3 Uncharted2ToneMapping(vec3 color) {
  const float A = 0.15; // Shoulder Strength
  const float B = 0.50; // Linear Strength
  const float C = 0.10; // Linear Angle
  const float D = 0.20; // Toe Strength
  const float E = 0.02; // Toe Numerator
  const float F = 0.30; // Toe Denominator
  const float W = 11.2; // Linear White Point value
  const float exposure = 2.0;
  
  color *= exposure;
  color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
  const float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
  color /= white;
  color = gamma_correct(color);
  return color;
}

// ----------------------------------------------------------------------------

vec3 testingToneMapping(vec3 color, float dx) {
  const float dn = 1.0 / 7.0;

  if (dx < 1.0f * dn) {
    return linearToneMapping(color);
  } 

  if (dx < 2.0f * dn) {
    return simpleReinhardToneMapping(color);
  } 

  if (dx < 3.0f * dn) {
    return lumaBasedReinhardToneMapping(color);
  } 

  if (dx < 4.0f * dn) {
    return whitePreservingLumaBasedReinhardToneMapping(color);
  } 

  if (dx < 5.0f * dn) {
    return RomBinDaHouseToneMapping(color);
  } 

  if (dx < 6.0f * dn) {
    return filmicToneMapping(color);
  } 

  return Uncharted2ToneMapping(color);
}

// ----------------------------------------------------------------------------

vec3 toneMapping(int mode, in vec3 color) {
  switch (mode) {
    case TONEMAPPING_LINEAR:
      return linearToneMapping(color); 

    case TONEMAPPING_SIMPLE_REINHARD:
      return simpleReinhardToneMapping(color);
     
    case TONEMAPPING_LUMA_REINHARD:
      return lumaBasedReinhardToneMapping(color);

    case TONEMAPPING_LUMA_REINHARD_2:
      return whitePreservingLumaBasedReinhardToneMapping(color);

    case TONEMAPPING_ROMBINDAHOUSE:
      return RomBinDaHouseToneMapping(color);

    case TONEMAPPING_FILMIC:
      return filmicToneMapping(color);

    case TONEMAPPING_UNCHARTED:
      return Uncharted2ToneMapping(color);
     
    default:
    case TONEMAPPING_NONE:
      return color;
  }

  return color;
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_INC_TONEMAPPING_GLSL_
