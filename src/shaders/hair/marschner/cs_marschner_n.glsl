#version 430 core

// ----------------------------------------------------------------------------
//
// Implements the azimuthal scaterring functions (« N* ») for the Marschner's 
// reflectance model.
//
//      ref : "Light Scaterring from Human Hair Fibers" [Marschner et al. 2003]
//
// ----------------------------------------------------------------------------

precision highp float;

#include "hair/marschner/inc_marschner_n.glsl"

// ----------------------------------------------------------------------------

uniform float uInvResolution;
writeonly uniform layout(rgba16f) image2D uDstImg;

#ifndef BLOCK_DIM
  #define BLOCK_DIM      16
#endif

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM) in;
void main()
{
  // Input parameters.
  const float kRefraction = uEta;          //< refraction index.
  const float kAbsorption = uAbsorption;   //< absorption coefficient.

  // ---------------------------------------------

  const int x = int(gl_GlobalInvocationID.x);
  const int y = int(gl_GlobalInvocationID.y);

  const float cosPhiD   = 2.0 * x * uInvResolution - 1.0;
  const float cosThetaD = 2.0 * y * uInvResolution - 1.0;

  // Use the Miller-Bravais index to find the Fresnel indices [cf Appendix B].
  const float sinThetaDSquared  = 1.0 - pow(cosThetaD, 2);
  const float refractionSquared = kRefraction * kRefraction;
  const float etaPerpendicular  = sqrt(refractionSquared - sinThetaDSquared) / cosThetaD;
  const float etaParallel       = refractionSquared / etaPerpendicular;

  const float phiD = acos(cosPhiD);
  const float c    = asin(1.0 / etaPerpendicular);

  // ---------------------------------------------

#if 1   // (generic version)

  // Surface Reflection.
  const float R   = Np(0, kRefraction, kAbsorption, etaPerpendicular, etaParallel, c, phiD);
  
  // Refractive Transmission.
  const float TT  = Np(1, kRefraction, kAbsorption, etaPerpendicular, etaParallel, c, phiD);
  
  // Internal Reflection (without caustics).
  const float TRT = Np(2, kRefraction, kAbsorption, etaPerpendicular, etaParallel, c, phiD);

#else   // (specialized version)

  // Surface Reflection.
  const float R   = NR(etaPerpendicular, etaParallel, phiD);
  
  // Refractive Transmission.
  const float TT  = NTT(kRefraction, kAbsorption, etaPerpendicular, etaParallel, c, phiD);
  
  // Internal Reflection. [ WIP ]
  // @todo: implement eccentricity for TRT.
  const float TRT = NTRT(kRefraction, kAbsorption, etaPerpendicular, etaParallel, c, phiD);

#endif

  // ---------------------------------------------

  const float kDebugSaturate = 1.0f;
  const vec4 kEnables = vec4(1, 1, 1, 1);
  const vec4 kScale = vec4(1, 1, 1, 1) * kEnables * kDebugSaturate;

  // Outputs are in [0.0, 1.0].
  imageStore( uDstImg, ivec2(gl_GlobalInvocationID.xy), kScale*vec4(R, TT, TRT, 1.0));
}

// ----------------------------------------------------------------------------
