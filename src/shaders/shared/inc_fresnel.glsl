#ifndef SHADERS_INC_FRESNEL_GLSL_
#define SHADERS_INC_FRESNEL_GLSL_

// ----------------------------------------------------------------------------
//
//  The Fresnel equations describe the reflection and transmission of light 
//   when incident on an interface between different optical media.
//
//    https://en.wikipedia.org/wiki/Fresnel_equations
//    https://en.wikipedia.org/wiki/Snell's_law
//
// ----------------------------------------------------------------------------

float fresnel_power_ratio(float _etaRatio, float _nA, float _nB, float _cosI, float _sinI) {
  const float sinTSquared = pow(_etaRatio * _sinI, 2); // (derived from Snell's law)
  if (sinTSquared > 1.0) {
    return 1.0;
  }
  const float cosT = sqrt(1 - sinTSquared);
  const float A = _nA * _cosI;
  const float B = _nB * cosT;
  const float R = (A - B) / (A + B);
  return min(1.0, R * R);
}

// ----------------------------------------------------------------------------

// Fresnel reflective ratio for perpendicular surfaces.
float fresnel_reflective_ratio(float n1, float n2, float cosAngle, float sinAngle) {
  return fresnel_power_ratio(n1 / n2, n1, n2, cosAngle, sinAngle);
}

float fresnel_reflective_ratio(float n1, float n2, float angle) {
  return fresnel_reflective_ratio(n1, n2, cos(angle), sin(angle));
}

// ----------------------------------------------------------------------------

// Fresnel transmitive ratio for parallel surfaces.
float fresnel_transmittance_ratio(float n1, float n2, float cosAngle, float sinAngle) {
  return fresnel_power_ratio(n1 / n2, n2, n1, cosAngle, sinAngle);
}

float fresnel_transmittance_ratio(float n1, float n2, float angle) {
  return fresnel_transmittance_ratio(n1, n2, cos(angle), sin(angle));
}

// ----------------------------------------------------------------------------

// Return a Fresnel coefficient for unpolarized surfaces via trigonometric values.
float fresnel(float etaOrigin, float etaPerpendicular, float etaParallel, float cosAngle, float sinAngle) {
  return mix(
    fresnel_reflective_ratio   ( etaOrigin, etaPerpendicular, cosAngle, sinAngle ), 
    fresnel_transmittance_ratio( etaOrigin,      etaParallel, cosAngle, sinAngle ), 
    0.5
  );
}

float fresnel(float etaOrigin, float etaPerpendicular, float etaParallel, float angle) {
  return fresnel(etaOrigin, etaPerpendicular, etaParallel, cos(angle), sin(angle));
}

// (using different incident / transmitive angles).
// float fresnel2(float etaOrigin, float etaPerpendicular, float etaParallel, float theta_i, float theta_t) {
//   return mix(
//     fresnel_reflectivity_ratio ( etaOrigin, etaPerpendicular, theta_i ), 
//     fresnel_transmittance_ratio( etaOrigin,      etaParallel, theta_t ), 
//     0.5
//   );
// }

// ----------------------------------------------------------------------------

#endif // SHADERS_INC_FRESNEL_GLSL_
