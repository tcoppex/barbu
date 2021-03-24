#version 430 core

// ----------------------------------------------------------------------------
//
// Implements the longitudinal scaterring function (« M ») for the Marschner's 
// reflectance model.
//
//      ref : "Light Scaterring from Human Hair Fibers" [Marschner et al. 2003]
//
// ----------------------------------------------------------------------------

precision highp float;

#include "shared/inc_maths.glsl"

// ----------------------------------------------------------------------------

uniform float uLongitudinalShift;
uniform float uLongitudinalWidth;

uniform float uInvResolution;

writeonly 
uniform layout(rgba16f) image2D uDstImg;

// ----------------------------------------------------------------------------

#ifndef BLOCK_DIM
  #define BLOCK_DIM      16
#endif

layout(local_size_x = BLOCK_DIM, local_size_y = BLOCK_DIM) in;
void main()
{
  const int x = int(gl_GlobalInvocationID.x);
  const int y = int(gl_GlobalInvocationID.y);

  const float sinThetaI = 2.0 * x * uInvResolution - 1.0;
  const float sinThetaR = 2.0 * y * uInvResolution - 1.0;
  
  const float thetaI = asin(sinThetaI);
  const float thetaR = asin(sinThetaR);
  const float thetaH = (thetaI + thetaR) / 2;
  const float thetaD = (thetaI - thetaR) / 2;

  const vec3 shifts = vec3( 1.0, -0.5, -1.5 ) * uLongitudinalShift;
  const vec3 widths = vec3( 1.0, +0.5, +2.0 ) * uLongitudinalWidth;

  // (we can use any other lobe function here)
  const vec3 lobes = gaussian( widths, vec3(degrees(thetaH)) - shifts);

  const float cosThetaD = sinThetaI;//cos( thetaD ); // sinThetaI

  // --------------------------

  // [normalize output ?]
  //atomicMax on shared buffer then on thread on global buffer

  const float kDebugSaturate = 1.0f;

  imageStore(uDstImg, ivec2(gl_GlobalInvocationID.xy), kDebugSaturate*vec4(lobes, cosThetaD));
}

// ----------------------------------------------------------------------------
