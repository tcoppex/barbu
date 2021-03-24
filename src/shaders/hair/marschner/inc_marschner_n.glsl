#ifndef SHADER_HAIR_INC_MARSCHNER_N_GLSL_
#define SHADER_HAIR_INC_MARSCHNER_N_GLSL_

#include "shared/inc_maths.glsl"
#include "shared/inc_fresnel.glsl"
#include "shared/inc_solver.glsl"

// ----------------------------------------------------------------------------

// Parameters for the shading model.
//uniform ShadingParam_t {
  // Fiber Properties.
  uniform float uEta            ;
  uniform float uAbsorption     ;
  uniform float uEccentricity   ;

  // Glints.
  uniform float uGlintScale     ;
  uniform float uAzimuthalWidth ;
  uniform float uDeltaCaustic   ;
  uniform float uDeltaHm        ;
//};

// ----------------------------------------------------------------------------

// Fresnel index of the viewer / light origin media (air = 1.0).
const float kEtaOrigin = 1.0;

// ----------------------------------------------------------------------------

// Coefficients of the cubic polynomial to solve [cf Equation 10].
vec4 get_polynomial_coefficients(int _p, float _c, float _phi) {
  const float kMinusEightOverPiCube = - 0.25801227547;
  const float kSixOverPi            = + 1.90985931710;
  return vec4(
    _p * _c * kMinusEightOverPiCube,
    0,
    _p * _c * kSixOverPi - 2.0,
    _p * Pi() - _phi
  );
}

// ----------------------------------------------------------------------------

// Returns the polynomial first derivative to h for gamma [cf Eq. 8, section 4.3].
// see : https://www.wolframalpha.com/input/?i=Aasin%28x%29%5E3%2BCasin%28x%29%2BD
float first_derivative_result(in vec4 _coeffs, float _gammaI, float _cosGammaI) {
  return (3 * _coeffs.x * pow(_gammaI, 2) + _coeffs.z) / _cosGammaI;
}

float inv_first_derivative_factor(in vec4 _coeffs, float _gammaI, float _cosGammaI) {
  return 1.0f / abs(2.0 * first_derivative_result(_coeffs, _gammaI, _cosGammaI));
}

// ----------------------------------------------------------------------------

// Returns the polynomial second derivative to h result for gamma.
// https://www.wolframalpha.com/input/?i=%28C+%2B+3+A+sin%5E%28-1%29%28x%29%5E2%29%2Fsqrt%281+-+x%5E2%29
float second_derivative_result(in vec4 _coeffs, float _gammaI, float _cosGammaI) {
  return ((3 * _coeffs.x * _gammaI) * (_gammaI + 2*_cosGammaI) + _coeffs.z) / pow(_cosGammaI, 3);
}

// ----------------------------------------------------------------------------

// Calculate A(p, h) [cf section 4.3].
float calculate_absorption(
  int _p,
  float _refraction,
  float _absorption,
  float _etaPerpendicular, 
  float _etaParallel,
  float _cosGammaI,
  float _sinGammaI) 
{
  // [Could be commented if we use the specialized functions instead].
  if (0 == _p) {
    return fresnel( kEtaOrigin, _etaPerpendicular, _etaParallel, _cosGammaI, _sinGammaI);
  }

  // gammaT is the angle of the refracted ray.
  const float sinGammaT = _sinGammaI / _etaPerpendicular;
  const float gammaT    = asin(sinGammaT);
  const float cosGammaT = cos(gammaT);

  const float fresnelCoeff_i    = fresnel( kEtaOrigin,     _etaPerpendicular,     _etaParallel, _cosGammaI, _sinGammaI);
  const float fresnelCoeffInv_t = fresnel( kEtaOrigin, 1 / _etaPerpendicular, 1 / _etaParallel,  cosGammaT,  sinGammaT);

  // Calculate T(sigma_a, h).
  const float t = exp(-4.0 * _absorption * pow(cosGammaT, 2));

  return pow(1 - fresnelCoeff_i, 2) 
       * pow(fresnelCoeffInv_t, _p - 1) 
       * pow(t, _p)
       ;
}

// ----------------------------------------------------------------------------

// Generic Normal-Plane Scaterring function [cf Eq. 8, section 4.3].
float Np(
  int _p,
  float _refraction, 
  float _absorption, 
  float _etaPerpendicular,
  float _etaParallel,
  float _c,
  float _phi) 
{
  // Find perpendicular roots [cf Eq. 10, section 5.2.1].
  const vec4 coeffs = get_polynomial_coefficients( _p, _c, _phi);
  const vec4 roots  = solver_polynomial(coeffs);
  const int nRoots  = int(roots.w);

  float L = 0.0;
  for (int i = 0; i < nRoots; ++i) {
    // gammaI is the angle of the incident ray.
    const float gammaI = roots[i];

    // sinGammaI ('h' in the paper) is the incident ray offset from the cross-section center
    const float sinGammaI = sin(gammaI);
    const float cosGammaI = cos(gammaI);

    const float a = calculate_absorption( 
      _p, _refraction, _absorption, _etaPerpendicular, _etaParallel, cosGammaI, sinGammaI
    );
    L += a * inv_first_derivative_factor(coeffs, gammaI, cosGammaI); 
  }

  return min(L, 1.0);
}

// ----------------------------------------------------------------------------

// Simplification for Np(0) (Surface Reflection).
float NR(
  float _etaPerpendicular,
  float _etaParallel,
  float _phi) 
{
  const float gammaI = - _phi / 2.0;
  const float cosGammaI = cos(gammaI);
  const float sinGammaI = sin(gammaI);
  const float a = fresnel( kEtaOrigin, _etaPerpendicular, _etaParallel, cosGammaI, sinGammaI);
  const float L = a * abs(cosGammaI / 2.0);
  
  return min(L, 1.0);
}

// ----------------------------------------------------------------------------

// Simplification for Np(1) (Refractive Transmission).
float NTT(
  float _refraction, 
  float _absorption, 
  float _etaPerpendicular,
  float _etaParallel,
  float _c,
  float _phi) 
{
  // Find perpendicular roots [cf Eq. 10, section 5.2.1].
  const vec4 coeffs = get_polynomial_coefficients( 1, _c, _phi);
  const vec4 roots = solver_polynomial(coeffs);

  // As Np(1) has only one root we bypass the loop [cf section 5.2.1].
  const float gammaI = roots.x;
  const float cosGammaI = cos(gammaI);
  const float sinGammaI = sin(gammaI);
  
  const float a = calculate_absorption( 
    1, _refraction, _absorption, _etaPerpendicular, _etaParallel, cosGammaI, sinGammaI
  ); 
  const float L = a * inv_first_derivative_factor(coeffs, gammaI, cosGammaI); 

  return min(L, 1.0);
}

// ----------------------------------------------------------------------------

float _inv_second_derivative(vec4 _dcoeffs, float _gammaI, float _h) {
  const vec4 ddcoeffs = derivate_coefficients(_dcoeffs);

  const float df  = polynomial_result( _dcoeffs, _gammaI);
  const float ddf = polynomial_result( ddcoeffs, _gammaI);

  const float dg  = max( sqrt(1 - _h * _h), Epsilon());
  const float ddg = _h / dg;

  const float A = ddf * dg - df * ddg;
  const float B = dg * dg;

  return B / A;
}

// Complete azimuthal scaterring function for Np(2) (Internal Reflection with caustics).
float NTRT(
  float _refraction, 
  float _absorption, 
  float _etaPerpendicular,
  float _etaParallel,
  float _c,
  float _phi) 
{
  float h_c;
  float phi_c;
  float delta_h;
  float t;

  const float wc = radians(uAzimuthalWidth);
  const vec4 coeffs = get_polynomial_coefficients( 2, _c, _phi);

  // Caustics occurs when the perpendicular index is below 2.
  if (_etaPerpendicular < 2.0) {

    // [wip - not sure about the equation]
    //-------------
    // first derivative coefficients.
    const vec4 dcoeffs = derivate_coefficients(coeffs);
    float gamma;
#if 0
    h_c = sqrt((4.0 - pow(_etaPerpendicular, 2)) / 3.0);
    gamma = asin(h_c);
    //phi_c = 4 * asin(h_c / _etaPerpendicular) - 2 * gamma + 2 * Pi();
#else
    h_c = solver_polynomial(dcoeffs).x;
    gamma = asin(h_c);
#endif
    phi_c = polynomial_result(coeffs, h_c);
    //-------------

    const float inv_ddh = _inv_second_derivative( dcoeffs, gamma, h_c); //
    delta_h = 2 * sqrt(2 * wc * abs(inv_ddh));
    delta_h = min( uDeltaHm, delta_h);
    t = 1.0;
  } else {
    phi_c = 0;
    delta_h = uDeltaHm;
    t = 1.0 - smoothstep(0.0, uDeltaCaustic, _etaPerpendicular - 2.0);
  }

  const float gammaI    = solver_polynomial(coeffs).x;
  const float cosGammaI = cos(gammaI);
  const float sinGammaI = sin(gammaI);

  const float dG0 = 1.0 / gaussian( wc, 0.0);
  const float gauss_l = gaussian( wc, _phi - phi_c);
  const float gauss_r = gaussian( wc, _phi + phi_c);

  const float a = calculate_absorption( 
    2, _refraction, _absorption, _etaPerpendicular, _etaParallel, cosGammaI, sinGammaI
  );

  float L = Np(2, _refraction, _absorption, _etaPerpendicular, _etaParallel, _c, _phi);
  L *= (1.0 - t * gauss_l) * dG0;
  L *= (1.0 - t * gauss_r) * dG0;
  L += t * uGlintScale * a * delta_h * (gauss_l + gauss_r);

  return min(L, 1);
}

// ----------------------------------------------------------------------------

#endif // SHADER_HAIR_INC_MARSCHNER_N_GLSL_
