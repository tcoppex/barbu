#ifndef SHADERS_SHARED_INC_GAMMA_GLSL_
#define SHADERS_SHARED_INC_GAMMA_GLSL_

// ----------------------------------------------------------------------------

// [should be user defined]
const float kGamma    = 2.2;
const float kInvGamma = 1.0 / kGamma;

// ----------------------------------------------------------------------------

float gamma_correct(in float x) {
  return pow( x, kInvGamma);
}

float gamma_uncorrect(in float x) {
  return pow( x, kGamma);
}

vec3 gamma_correct(in vec3 rgb) {
  return pow( rgb, vec3(kInvGamma));
}

vec3 gamma_uncorrect(in vec3 rgb) {
  return pow( rgb, vec3(kGamma));
}

// ----------------------------------------------------------------------------

#endif // SHADERS_SHARED_INC_GAMMA_GLSL_